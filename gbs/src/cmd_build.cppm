module;
#include <algorithm>
#include <coroutine>
#include <execution>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <locale>
#include <mutex>
#include <print>
#include <ranges>
#include <set>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
export module cmd_build;
import env;
import context;
import get_source_groups;
import cmd_config;
import os;
import dep_scan;
import task;
import task_graph;

namespace fs = std::filesystem;
using imports_map = std::unordered_map<fs::path, import_set>;  // source -> {imports}
using module_map = std::unordered_map<std::string, fs::path>;  // import -> source

// Why doesn't this garbage stl have this already???
static std::string to_upper(std::string const& cstr) {
	std::string str = cstr;
	std::use_facet<std::ctype<char>>(std::locale()).toupper(str.data(), str.data() + str.size());
	return str;
}

static bool is_file_out_of_date(fs::path const& path, fs::path const& obj) {
	if (!fs::exists(obj))
		return true;

	return (fs::last_write_time(obj) <= fs::last_write_time(path));
}

static fs::path get_object_filepath(fs::path const& path, context const& ctx) {
	fs::path const obj = (ctx.output_dir() / path.filename()).replace_extension("obj");
	return obj;
}

static auto make_build_job(context const& ctx, fs::path const& path, fs::path const& obj, std::string_view defines) {
	auto cmd =
		ctx.build_command_prefix() +
		ctx.build_command(path.generic_string(), obj) +
		ctx.get_response_args().data() +
		ctx.get_module_directory();
	if (!defines.empty())
		cmd += ctx.build_define(defines);
	return [cmd = std::move(cmd)] { std::system(cmd.c_str()); };
}

static bool init_build(context& ctx) {
	// Bail if no compiler is selected
	auto const& selected_cl = ctx.get_selected_compiler();
	if (selected_cl.name.empty()) {
		std::println(std::cerr, "<gbs> No compiler selected/found.");
		return false;
	}

	// Set the default build configuration if not specified
	if (ctx.get_config().empty())
		if (!cmd_config(ctx, "debug,warnings"))
			return false;

	// Set the target operating system in the context
	if (ctx.get_selected_compiler().name == "msvc") {
		ctx.set_target_os(operating_system::windows);
	}
	else {
		std::string const base_name = fs::current_path().stem().generic_string();
		auto const cmd = ctx.build_command_prefix() + ctx.get_response_args().data() + " -dumpmachine > arch.txt";
		std::string arch;
		if (0 == std::system(cmd.c_str()) && std::getline(std::ifstream("arch.txt"), arch)) {
			std::remove("arch.txt");
			ctx.set_target_os(os_from_target_triple(arch));
		}
		else {
			std::println(std::cerr, "<gbs> Unable to determine target os.");
			return false;
		}
	}

	// Make sure the output and response directories exist
	fs::create_directories(ctx.output_dir());

#ifdef _MSC_VER
	// Initialize msvc environment
	if (selected_cl.name == "msvc" /*|| selected_cl.name == "clang"*/) {
		extern bool init_msvc(context const&);
		if (!init_msvc(ctx))
			return false;
	}
#endif

	return true;
}

static bool is_valid_sourcefile(fs::path const& file) {
	static constexpr std::array<std::string_view, 4> extensions{ ".cpp", ".c", ".cppm", ".ixx" };
	return extensions.end() != std::find(extensions.begin(), extensions.end(), file.extension());
}

// Create the object list file
static fs::path create_object_file_list(context& ctx, std::string_view name, std::span<const fs::path> paths) {
	fs::path objlist_name = (ctx.output_dir() / name).concat("_OBJLIST");
	std::ofstream objlist(objlist_name);
	for(fs::path const& src : paths) {
		fs::path const obj = (ctx.output_dir() / src.filename()).replace_extension("obj");
		objlist << obj.generic_string() << ' ';
	}
	objlist.close();
	return objlist_name;
}

static task_ptr create_build_task(context const& ctx, task_graph& tg, fs::path const& path, module_map& modmap, imports_map& impmap, std::string_view defines = "") {
	if (!is_valid_sourcefile(path) || !should_include(path))
		return {};

	source_dependency const deps = extract_module_dependencies(path);
	if (deps.is_export())
		modmap[deps.export_name] = path;
	impmap[path] = deps.import_names;

	fs::path const obj = get_object_filepath(path, ctx);
	if (is_file_out_of_date(path, obj)) {
		return tg.create_task(path, make_build_job(ctx, path, obj, defines));
	}
	return {};
}

export bool cmd_build(context& ctx, std::string_view /*target*/) {
	std::println("<gbs> Building...");

	if (!init_build(ctx))
		return false;

	// Save paths to libraries and objects
	std::set<fs::path> libs;
	std::set<fs::path> objects;

	// Containers for all source files, includes, defines and targets
	std::unordered_set<fs::path> includes;
	module_map modmap;
	imports_map impmap;
	task_graph graph;

	// Add the std module to the build
	fs::path const std_module_path = *ctx.get_selected_compiler().std_module;
	create_build_task(ctx, graph, std_module_path, modmap, impmap);

	// 'lib' directory: process all libraries shared between all the projects
	auto lib_task = graph.create_task("lib", []() {});
	for (auto dir_it : fs::directory_iterator(".", fs::directory_options::follow_directory_symlink | fs::directory_options::skip_permission_denied)) {
		if (!dir_it.is_directory())
			continue;

		fs::path const p = dir_it.path().lexically_normal();
		if (!should_include(p))
			continue;

		if ("src" == p) {
			std::println(std::cerr, "<gbs> error: 'src' directory shall be inside a project directory, aborting...");
			std::println(std::cerr, "             '{}'", fs::current_path().generic_string());
			return false;
		}

		if ("lib" == p) {
			for (fs::directory_entry const& dir : fs::directory_iterator("lib")) {
				if (!dir.is_directory())
					continue;

				fs::path const lib = dir.path().lexically_normal();
				if (fs::exists(lib / "src")) includes.insert(lib / "src");
				if (fs::exists(lib / "inc")) includes.insert(lib / "inc");
				if (fs::exists(lib / "include")) includes.insert(lib / "include");

				if (!lib.has_extension()) {
					includes.insert(lib);
					continue;
				}

				if (lib.stem() == "s") {
					for (fs::path const& path : get_source_files(lib)) {
						if (should_include(path)) {
							auto task = create_build_task(ctx, graph, path, modmap, impmap);
							if (task)
								graph.add_dependency(task, lib_task);
							objects.insert((ctx.output_dir() / path.filename()).replace_extension("obj"));
						}
					}
				}
				else if (lib.stem() == "d") {
					std::string const export_define = to_upper(lib.extension().generic_string().substr(1)) + "_EXPORTS";
					auto const vec = get_source_files(lib) | std::ranges::to<std::vector>();

					// Create the object list file for the .lib file
					std::string const name = lib.extension().generic_string().substr(1);
					auto const objlist_name = create_object_file_list(ctx, name, vec);

					// Add it to the library list
					fs::path const out_lib = ctx.output_dir() / os_get_static_library_name(ctx.get_target_os(), name);
					libs.insert(out_lib);

					auto dll_task = graph.create_task(lib, [&ctx, name, objlist_name] {
						std::string const lib_name = os_get_static_library_name(ctx.get_target_os(), name);
						std::string const dll_name = os_get_dynamic_library_name(ctx.get_target_os(), name);

						std::println("<gbs> Creating dynamic library '{}'...", dll_name);
						std::string const obj_resp = std::format(" @{}", objlist_name.generic_string());
						std::string const cmd = ctx.dynamic_library_command(dll_name, lib_name, ctx.output_dir().generic_string()) + obj_resp;
						std::system(cmd.c_str());
						});

					graph.add_dependency(dll_task, lib_task);
					for (fs::path const& path : vec) {
						if (should_include(path)) {
							auto src_task = create_build_task(ctx, graph, path, modmap, impmap, export_define);
							if (src_task)
								graph.add_dependency(src_task, dll_task);
						}
					}
				}
				else {
					std::println("<gbs> warning: skipping directory '{}' in 'lib' since it doesn't follow naming convention (s.* for static libs, d.* for dynamic libs)", lib.generic_string());
				}
			}
		}
		else {
			if (fs::exists(p / "src")) {
				auto const source_files = get_source_files(p / "src") | std::ranges::to<std::vector>();

				// Create the object list file for the .lib file
				std::string const name = p == "." ? fs::current_path().stem().generic_string() : p.stem().generic_string();
				auto const objlist_name = create_object_file_list(ctx, name, source_files);

				auto exe_task = graph.create_task(p, [&ctx, name, objlist_name] {
					std::string const exe_name = os_get_executable_name(ctx.get_target_os(), name);

					std::println("<gbs> Linking executable '{}'...", exe_name);
					std::string const obj_resp = std::format(" @{}", objlist_name.generic_string());
					std::string const cmd = ctx.link_command(exe_name, ctx.output_dir().generic_string()) + obj_resp;
					std::system(cmd.c_str());
					});

				//graph.add_dependency(lib_task, exe_task);
				for (fs::path const& path : source_files) {
					if (should_include(path)) {
						auto src_task = create_build_task(ctx, graph, path, modmap, impmap);
						if (src_task) {
							graph.add_dependency(lib_task, src_task);
							graph.add_dependency(src_task, exe_task);
						}
					}
				}
			}

			if (fs::exists(p / "unittest")) {
				auto source_files = get_source_files(p / "unittest") | std::ranges::to<std::vector>();

				std::string const name = p.stem().generic_string();

				// Partition the source files into unittests and support files
				auto const unittests = std::ranges::partition(source_files, [](fs::path const& path) { return !path.filename().generic_string().starts_with("test."); });
				auto const supports = std::ranges::subrange(source_files.begin(), unittests.begin());

				// Create the object list file for non-test files
				fs::path objlist_name = create_object_file_list(ctx, "sup_" + name, supports);

				// Create the build tasks for the support files
				std::vector<task_ptr> support_tasks;
				for (fs::path const& path : supports) {
					if (should_include(path)) {
						auto src_task = create_build_task(ctx, graph, path, modmap, impmap);
						if (src_task) {
							support_tasks.push_back(std::move(src_task));
						}
					}
				}

				// Link each unittest
				std::vector<task_ptr> unittest_tasks;
				for(fs::path const& test : unittests) {
					std::string const test_name = test.stem().generic_string();
					std::string const exe_name = os_get_executable_name(ctx.get_target_os(), test_name);

					// Save the unittest executable in the context
					ctx.add_unittest(ctx.output_dir() / exe_name);

					// Create the unittest task
					auto exe_task = graph.create_task(test, [&ctx, test_name, exe_name, objlist_name] {
						std::println("<gbs> Linking unittest '{}'...", exe_name);
						std::string const obj_resp = std::format(" @{} {}/{}.obj", objlist_name.generic_string(), ctx.output_dir().generic_string(), test_name);
						std::string const cmd = ctx.link_command(exe_name, ctx.output_dir().generic_string()) + obj_resp;
						std::system(cmd.c_str());
						});

					auto src_task = create_build_task(ctx, graph, test, modmap, impmap);
					graph.add_dependency(lib_task, src_task);
					graph.add_dependency(src_task, exe_task);
					for (auto const& support_task : support_tasks) {
						//graph.add_dependency(lib_task, support_task);
						graph.add_dependency(support_task, exe_task);
					}
				}
			}
		}
	}

	// Create the object list file
	{
		std::ofstream objlist(ctx.output_dir() / "OBJLIST");
		for (fs::path const& obj : objects)
			objlist << obj.generic_string() << ' ';
		objlist.close();
	}

	// Create a response file for all include paths
	{
		std::ofstream includes_rsp(ctx.output_dir() / "SRC_INCLUDES");
		for (fs::path const& include : includes)
			includes_rsp << ctx.make_include_path(include.generic_string()) << ' ';
		includes_rsp.close();
	}

	// Create the library list file
	std::ofstream liblist(ctx.output_dir() / "LIBLIST");
	for (fs::path const& lib : libs)
		liblist << lib.generic_string() << ' ';
	liblist.close();

	// Set up the task dependencies for modules
	for (auto const& [path, impset] : impmap) {
		task_ptr const& task = graph.find_task(path);
		for (auto const& imp : impset)
		{
			if (modmap.contains(imp)) {
				auto const& dep_path = modmap.at(imp);
				auto dep_task = graph.find_task(dep_path);
				if (dep_task) {
					//std::println("<gbs> linking '{}' -> '{}'", path.generic_string(), dep_path.generic_string());
					graph.add_dependency(dep_task, task);
				}
			}
			else {
				std::println("<gbs> module '{}' imported by '{}' not found", imp, path.generic_string());
				//return false;
			}
		}
	}

	graph.run();

	return true;
}
