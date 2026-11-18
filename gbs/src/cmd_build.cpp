#include "../inc/commands.h"
#include "../inc/context.h"
#include "../inc/dep_scan.h"
#include "../inc/env.h"
#include "../inc/get_source_files.h"
#include "../inc/os.h"
#include "../inc/task/task_graph.h"
#include <algorithm>
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

namespace fs = std::filesystem;
using namespace std::views;

template<typename R, typename T>
concept range_of = std::ranges::range<R> && std::same_as<std::ranges::range_value_t<R>, T>;

using imports_map = std::unordered_map<fs::path, import_set>;	// source -> {imports}
using module_map = std::unordered_map<module_desc, fs::path>;	// import -> source
using task_map = std::unordered_map<fs::path, task_ptr>;		// source -> task

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

static bool is_valid_sourcefile(fs::path const& file) {
	static constexpr std::array<std::string_view, 5> extensions{ ".cpp", ".cc", ".c", ".cppm", ".ixx" };
	return extensions.end() != std::find(extensions.begin(), extensions.end(), file.extension());
}

static bool is_valid_project_name(fs::path const& p) {
	// Ignore temporary directories and the 'out' directory
	return p.c_str()[0] != '.'
		&& *p.begin() != "out"
		&& *p.begin() != "gbs.out";
}

static void create_response_file(context const& ctx, std::string_view name, std::ranges::range auto&& content) {
	std::ofstream objlist(ctx.output_dir() / name);
	for (auto const& obj : content)
		objlist << obj << ' ';
	objlist.close();
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
	return [cmd = std::move(cmd)] { return 0 == std::system(cmd.c_str()); };
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
	if (selected_cl.name == "msvc") {
		ctx.set_target_os(operating_system::windows);
	}
	else {
		std::string const base_name = fs::current_path().stem().generic_string();
		auto const cmd = ctx.build_command_prefix() + ctx.get_response_args().data() + " -dumpmachine > arch.txt";
		std::string arch;
		if (0 == std::system(cmd.c_str()) && std::getline(std::ifstream("arch.txt"), arch)) {
			std::filesystem::remove("arch.txt");
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
	if (selected_cl.name == "msvc" /*|| selected_cl.unittest_name == "clang"*/) {
		extern bool init_msvc(context const&);
		if (!init_msvc(ctx))
			return false;
	}
#endif

	return true;
}

// Create the object list file
static fs::path create_object_file_list(context& ctx, std::string_view name, range_of<fs::path> auto const& paths) {
	fs::path objlist_name = (ctx.output_dir() / name).concat("_OBJLIST");
	std::ofstream objlist(objlist_name);
	for(fs::path const& src : paths) {
		fs::path const obj = get_object_filepath(src, ctx);
		objlist << obj.generic_string() << ' ';
	}
	objlist.close();
	return objlist_name;
}

static task_ptr create_build_task(context const& ctx, task_graph& tg, fs::path const& path, module_map& modmap, imports_map& impmap, std::string_view defines = {}) {
	if (!is_valid_sourcefile(path) || !should_include(ctx, path))
		return {};

	source_dependency const deps = extract_module_dependencies(path);
	if (deps.is_export())
		modmap[deps.export_name] = path;
	impmap[path].insert_range(deps.import_names);

	fs::path const obj = get_object_filepath(path, ctx);
	if (is_file_out_of_date(path, obj)) {
		return tg.create_task(path, make_build_job(ctx, path, obj, defines));
	}
	return {};
}

static task_ptr create_static_lib(context const& ctx, task_graph& graph, fs::path project_dir, fs::path objlist_name, fs::file_time_type latest_source_time) {
	return graph.create_task(project_dir, [=, &ctx] {
		auto const ext_name = project_dir.extension().generic_string().substr(1);

		// static library, create the .lib
		std::string const lib_name = os_get_static_library_name(ctx.get_target_os(), ext_name);

		auto const lib_path = ctx.output_dir() / lib_name;
		if (fs::exists(lib_path) && latest_source_time < fs::last_write_time(lib_path))
			return true;

		std::string const cmd = ctx.static_library_command(lib_name, ctx.output_dir().generic_string()) + std::format(" @{}", objlist_name.generic_string());
		std::println("<gbs> Linking static library '{}'...", lib_name);
		return 0 == std::system(cmd.c_str());
		});
}

static task_ptr create_dynamic_lib(context const& ctx, task_graph& graph, fs::path project_dir, fs::path objlist_name, fs::file_time_type latest_source_time) {
	return graph.create_task(project_dir, [=, &ctx] {
		auto const ext_name = project_dir.extension().generic_string().substr(1);

		// dynamic library, link the .dll
		std::string const lib_name = os_get_static_library_name(ctx.get_target_os(), ext_name);
		std::string const dll_name = os_get_dynamic_library_name(ctx.get_target_os(), ext_name);

		auto const dll_path = ctx.output_dir() / dll_name;
		if (fs::exists(dll_path) && latest_source_time < fs::last_write_time(dll_path))
			return true;

		std::string const cmd = ctx.dynamic_library_command(dll_name, lib_name, ctx.output_dir().generic_string())
			+ std::format(" @{}", objlist_name.generic_string());
		std::println("<gbs> Linking dynamic library '{}'...", dll_name);
		return 0 == std::system(cmd.c_str());
		});
}

static task_ptr create_executeable(context const& ctx, task_graph& graph, std::string name, fs::path project_dir, fs::path objlist_name, fs::file_time_type latest_source_time) {
	return graph.create_task(project_dir, [=, &ctx] {
		std::string const main_obj_name = (ctx.output_dir() / name).replace_extension("obj").generic_string();
		std::string const exe_name = os_get_executable_name(ctx.get_target_os(), name);

		auto const exe_path = ctx.output_dir() / exe_name;
		if (fs::exists(exe_path) && latest_source_time < fs::last_write_time(exe_path))
			return true;

		std::println("<gbs> Linking executable '{}'...", exe_name);
		std::string const obj_resp = std::format(" @{} {}", objlist_name.generic_string(), main_obj_name);
		std::string const cmd = ctx.link_command(exe_name, ctx.output_dir().generic_string()) + obj_resp;
		return 0 == std::system(cmd.c_str());
		});
}

bool cmd_build(context& ctx, std::string_view /*target*/) {
	std::println("<gbs> Building...");

	if (!init_build(ctx))
		return false;

	// Save paths to libraries, objects, includes
	std::set<fs::path> libs;
	std::set<fs::path> objects;
	std::set<fs::path> includes;

	// Containers for module <-> path
	module_map modmap;
	imports_map impmap;

	// The task graph to hold all build tasks and their dependencies
	task_graph graph;

	// Add the std module to the build
	compiler const& selected_cl = ctx.get_selected_compiler();
	task_ptr std_module_task;
	if (selected_cl.std_module) {
		fs::path const std_module_path = *selected_cl.std_module;
		std_module_task = create_build_task(ctx, graph, std_module_path, modmap, impmap);
		objects.insert((ctx.output_dir() / std_module_path.filename()).replace_extension("obj"));
	} else {
		std::println("<gbs> warning: '{}' does not provide a standard library module, builds may fail if the standard library is used", selected_cl.name_and_version);
	}

	// 'lib' directory: process all libraries shared between all the projects
	auto lib_barrier_task = graph.create_task("lib", task_true);

	if (fs::exists("lib")) {
		for (fs::directory_entry const& dir : fs::directory_iterator("lib", fs::directory_options::follow_directory_symlink | fs::directory_options::skip_permission_denied)) {
			if (!dir.is_directory())
				continue;

			fs::path const lib = dir.path().lexically_normal();
			//if (fs::exists(lib / "src")) includes.insert(lib / "src");
			if (fs::exists(lib / "inc")) includes.insert(lib / "inc");
			if (fs::exists(lib / "include")) includes.insert(lib / "include");

			if (!lib.has_extension()) {
				includes.insert(lib);
			}
			else if (lib.stem() == "s") {
				for (fs::path const& path : get_source_files(ctx, lib / "src")) {
					auto task = create_build_task(ctx, graph, path, modmap, impmap);
					graph.add_dependency(task, lib_barrier_task);
					objects.insert((ctx.output_dir() / path.filename()).replace_extension("obj"));
				}
			}
			else if (lib.stem() == "d") {
				std::string const export_define = to_upper(lib.extension().generic_string().substr(1)) + "_EXPORTS";

				auto const vec = get_source_files(ctx, lib) | std::ranges::to<std::vector>();
				std::string const name = lib.extension().generic_string().substr(1);

				// Create the object list file for the .lib file
				auto const objlist_name = create_object_file_list(ctx, name, vec);

				// Add it to the library list
				auto const lib_or_dll_name = (ctx.get_target_os() != operating_system::windows) ? os_get_dynamic_library_name(ctx.get_target_os(), name) : os_get_static_library_name(ctx.get_target_os(), name);
				fs::path const out_lib = ctx.output_dir() / lib_or_dll_name;// os_get_static_library_name(ctx.get_target_os(), unittest_name);
				libs.insert(out_lib);

				// Create the task to build the .dll file for this library
				auto const latest_source_time = vec.empty()
					? fs::file_time_type::min()
					: std::ranges::max(vec | transform([](fs::path const& src) { return fs::last_write_time(src); }));
				auto dll_task = create_dynamic_lib(ctx, graph, lib, objlist_name, latest_source_time);

				// Create the tasks for the source files and add them as dependencies to the .dll task
				for (fs::path const& path : vec) {
					auto src_task = create_build_task(ctx, graph, path, modmap, impmap, export_define);
					graph.add_dependency(src_task, dll_task);
				}

				graph.add_dependency(std_module_task, dll_task);
				graph.add_dependency(lib_barrier_task, dll_task);
			}
			else {
				std::println("<gbs> warning: skipping directory '{}' in 'lib' since it doesn't follow naming convention (s.* for static libs, d.* for dynamic libs)", lib.generic_string());
			}
		}
	}

	auto _it = fs::directory_iterator(".", fs::directory_options::follow_directory_symlink | fs::directory_options::skip_permission_denied)
		| filter([](fs::directory_entry e) { return e.is_directory(); })
		| transform(&fs::directory_entry::path)
		| transform(&fs::path::lexically_normal)
		| filter([&ctx](fs::path p) { return p != "lib" && should_include(ctx, p); });

	for (fs::path const& project_dir : _it) {
		fs::path objlist_name;

		if (fs::exists(project_dir / "src")) {
			fs::path const path = project_dir == "." ? fs::current_path() : project_dir;
			std::string const name = path.string().contains('.') ? path.extension().generic_string().substr(1) : path.stem().generic_string();

			// Create the object list file for the .lib file
			auto const source_files = get_source_files(ctx, project_dir / "src") | std::ranges::to<std::vector>();
			auto filter_main = filter([&](fs::path const& path_to_filter) {
				// Don't include the main source file in the object list, as it contains the 'main' function.
				return path_to_filter.filename().stem().generic_string() != name;
				});
			auto filtered_source_files = source_files | filter_main | std::ranges::to<std::vector>();
			objlist_name = create_object_file_list(ctx, name, filtered_source_files);

			auto const latest_source_time = source_files.empty()
				? fs::file_time_type::min()
				: std::ranges::max(source_files | transform([](fs::path const& src) { return fs::last_write_time(src); }));

			task_ptr project_link_task;
			if (path.string().starts_with("s.")) {
				project_link_task = create_static_lib(ctx, graph, project_dir, objlist_name, latest_source_time);
			}
			else if (path.string().starts_with("d.")) {
				project_link_task = create_dynamic_lib(ctx, graph, project_dir, objlist_name, latest_source_time);
			}
			else {
				project_link_task = create_executeable(ctx, graph, name, project_dir, objlist_name, latest_source_time);
			}

			for (fs::path const& source_path : source_files) {
				auto src_task = create_build_task(ctx, graph, source_path, modmap, impmap);
				graph.add_dependency(lib_barrier_task, src_task);
				graph.add_dependency(src_task, project_link_task);
			}
		}

		if (fs::exists(project_dir / "unittest")) {
			auto unittest_files = get_source_files(ctx, project_dir / "unittest") | std::ranges::to<std::vector>();

			std::string const unittest_name = project_dir.stem().generic_string();

			// Partition the source files into unittests and support files
			auto const unittests = std::ranges::partition(unittest_files, [](fs::path const& path) { return !path.filename().generic_string().starts_with("test."); });
			auto const supports = std::ranges::subrange(unittest_files.begin(), unittests.begin());

			// Create the object list file for non-test files
			fs::path sup_objlist_name = create_object_file_list(ctx, "sup_" + unittest_name, supports);

			// Create the build tasks for the support files
			task_ptr support_task = graph.create_task(project_dir / "unittest_support", task_true);
			for (fs::path const& support_path : supports) {
				if (should_include(ctx, support_path)) {
					auto src_task = create_build_task(ctx, graph, support_path, modmap, impmap);
					if (src_task) {
						graph.add_dependency(src_task, support_task);
					}
				}
			}
			graph.add_dependency(lib_barrier_task, support_task);

			// Link each unittest
			std::vector<task_ptr> unittest_tasks;
			for(fs::path const& test : unittests) {
				std::string const test_name = test.stem().generic_string();
				std::string const exe_name = os_get_executable_name(ctx.get_target_os(), test_name);

				// Save the unittest executable in the context
				ctx.add_unittest(ctx.output_dir() / exe_name);

				// Create the unittest task
				auto test_exe_task = graph.create_task(exe_name, [&ctx, test_name, exe_name, objlist_name, sup_objlist_name] mutable {
					std::println("<gbs> Linking unittest '{}'...", exe_name);
					std::string const obj_resp = std::format(" @{} @{} {}/{}.obj", objlist_name.generic_string(), sup_objlist_name.generic_string(), ctx.output_dir().generic_string(), test_name);
					std::string const cmd = ctx.link_command(exe_name, ctx.output_dir().generic_string()) + obj_resp;
					return 0 == std::system(cmd.c_str());
					});

				graph.add_dependency(support_task, test_exe_task);
				
				task_ptr exe_src_task = graph.find_task(project_dir);
				if (exe_src_task) {
					graph.add_dependency(exe_src_task, support_task);
				}

				if (auto src_task = create_build_task(ctx, graph, test, modmap, impmap); src_task) {
					graph.add_dependency(src_task, test_exe_task);
				}
			}
		}
	}

	// Setup module file dependencies
	for (auto const& [src, imports] : impmap) {
		if (imports.empty())
			continue;

		task_ptr src_task = graph.find_task(src);
		if (!src_task)
			continue;

		for (auto const& imp : imports) {
			if (!modmap.contains(imp))
				continue;

			auto const& imp_src = modmap[imp];
			task_ptr imp_task = graph.find_task(imp_src);
			if (imp_task)
				graph.add_dependency(imp_task, src_task);
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
		if (task) {
			for (auto const& imp : impset)
			{
				if (modmap.contains(imp)) {
					auto const& dep_path = modmap.at(imp);
					auto dep_task = graph.find_task(dep_path);
					if (dep_task) {
						//std::println("<gbs> linking '{}' -> '{}'", support_path.generic_string(), dep_path.generic_string());
						graph.add_dependency(dep_task, task);
					}
				}
				else {
					std::println("<gbs> build: module '{}' imported by '{}' not found", imp, path.generic_string());
					for (auto const& [mod, mod_path] : modmap) {
						std::println("<gbs>     available module: '{}' at '{}'", mod, mod_path.generic_string());
					}
					return false;
				}
			}
		}
	}

	graph.run();

	return !graph.was_aborted();
}

bool cmd_build_v2(context& ctx, std::string_view /*target*/) {
	std::println("<gbs> Building (v2) ...");

	if (!init_build(ctx))
		return false;

	// Save paths to libraries, objects, includes
	std::set<fs::path> libs;
	std::set<fs::path> objects;

	// Containers for module <-> path
	module_map modmap;
	imports_map impmap;
	std::unordered_map<fs::path, task_ptr> tasks;

	// The task graph to hold all build tasks and their dependencies
	task_graph graph;

	// Add the std module to the build
	compiler const& cl = ctx.get_selected_compiler();
	if (cl.std_module) {
		auto const& [obj, _] = objects.insert(get_object_filepath(*cl.std_module, ctx));
		if (is_file_out_of_date(*cl.std_module, *obj)) {
			tasks[*cl.std_module] = create_build_task(ctx, graph, *cl.std_module, modmap, impmap);
		}
	}
	else {
		std::println("<gbs> warning: '{}' does not provide a standard library module, builds may fail if the standard library is used", cl.name_and_version);
	}

	// Find all source files
	auto source_file_it = fs::recursive_directory_iterator(".", fs::directory_options::follow_directory_symlink)
		| filter([](fs::directory_entry const& e) { return e.is_regular_file(); })
		| transform(&fs::directory_entry::path)
		| transform(&fs::path::lexically_normal)
		| filter(is_valid_project_name)
		| filter(is_valid_sourcefile)
		| filter(std::bind_front(&should_include, std::cref(ctx)));
	for (fs::path path : source_file_it) {
		// Adjust the path for WSL if necessary
		if (cl.wsl && !std::filesystem::exists(path)) {
			std::string new_path = std::format(R"(\\wsl.localhost\{}{})", *cl.wsl, path.string());
			if (std::filesystem::exists(new_path)) {
				path = new_path;
			}
		}

		// Save library object paths
		std::string export_define;
		fs::path const obj = get_object_filepath(path, ctx);
		if (*path.begin() == "lib") {
			objects.insert(obj);

			fs::path const project_name = *std::next(path.begin());
			if (project_name.has_extension() && project_name.stem() == "d")
				export_define = to_upper(project_name.extension().generic_string().substr(1)) + "_EXPORTS";
		}

		// Extract module dependencies
		source_dependency const deps = extract_module_dependencies(path);
		if (deps.is_export())
			modmap[deps.export_name] = path;
		impmap[path].insert_range(deps.import_names);

		// Don't compile source files that are up to date
		if (!is_file_out_of_date(path, obj))
			continue;

		// Create the build task for this source file
		if (tasks.contains(path)) throw;
		tasks[path] = graph.create_task(path, make_build_job(ctx, path, obj, export_define));
	}

	// Set up the task dependencies for modules
	for (auto const& [path, impset] : impmap) {
		if (task_ptr const& task = graph.find_task(path); task) {
			for (auto const& imp : impset) {
				if (modmap.contains(imp)) {
					fs::path const& dep_path = modmap.at(imp);
					task_ptr const& dep_task = graph.find_task(dep_path);
					graph.add_dependency(dep_task, task);
				}
				else {
					std::println("<gbs> build: module '{}' imported by '{}' not found", imp, path.generic_string());
					for (auto const& [mod, mod_path] : modmap) {
						std::println("<gbs>     available module: '{}' at '{}'", mod, mod_path.generic_string());
					}
					return false;
				}
			}
		}
	}

	// Create the response files for compilation and linking
	create_response_file(ctx, "OBJLIST", objects);
	create_response_file(ctx, "LIBLIST", libs);
	create_response_file(ctx, "SRC_INCLUDES", fs::recursive_directory_iterator(".", fs::directory_options::follow_directory_symlink)
		| filter([](fs::directory_entry const& e) { return e.is_directory(); })
		| transform(&fs::directory_entry::path)
		| transform(&fs::path::lexically_normal)
		| filter([](fs::path const& p) { return p.filename() == "inc" || p.filename() == "include"; })
		| transform([&ctx](fs::path const& p) { return ctx.make_include_path(p); }));

	graph.run();

	return !graph.was_aborted();
}
