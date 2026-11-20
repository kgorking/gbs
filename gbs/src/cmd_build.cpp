#include "../inc/commands.h"
#include "../inc/context.h"
#include "../inc/dep_scan.h"
#include "../inc/env.h"
#include "../inc/get_source_files.h"
#include "../inc/os.h"
#include "../inc/task/task_graph.h"
#include <algorithm>
#include <filesystem>
#include <format>
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

constexpr std::array directories{ ".", "lib" /*, "tests" */ };

template<typename R, typename T>
concept range_of = std::ranges::range<R> && std::same_as<std::ranges::range_value_t<R>, T>;

using imports_map = std::unordered_map<fs::path, import_set>;	// source -> {imports}
using module_map = std::unordered_map<module_desc, fs::path>;	// import -> source
using task_map = std::unordered_map<fs::path, task_ptr>;		// source -> task

// Task names must be unique across all compilers sharing the same graph.
static fs::path make_task_name(fs::path const& prefix, fs::path const& name) {
	return fs::path{ std::format("{}/{}", prefix.generic_string(), name.generic_string()) };
}

static fs::path compiler_task_prefix(compiler const& cl) {
	return fs::path{ cl.name_and_version };
}

enum class project_type {
	unknown,
	static_lib,
	dynamic_lib,
	executable
};

static project_type get_project_type(fs::path const& project_dir) {
	if (!fs::exists(project_dir / "src"))
		return project_type::unknown;
	if (project_dir.filename().string().starts_with("s."))
		return project_type::static_lib;
	else if (project_dir.filename().string().starts_with("d."))
		return project_type::dynamic_lib;
	else
		return project_type::executable;
}

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
	return fs::exists(p / "src");
}

static void create_response_file(context const& ctx, compiler const& cl, std::string_view name, std::ranges::range auto&& content) {
	std::ofstream objlist(ctx.output_dir(cl) / name);
	for (auto const& obj : content)
		objlist << obj << ' ';
	objlist.close();
}

static fs::path get_object_filepath(fs::path const& path, context const& ctx, compiler const& cl) {
	fs::path const obj = (ctx.output_dir(cl) / path.filename()).replace_extension("obj");
	return obj;
}

static auto make_build_job(context const& ctx, compiler const& cl, fs::path const& path, fs::path const& obj, std::string_view defines) {
	std::string const out = ctx.output_dir(cl).generic_string();
	auto cmd =
		cl.build_command_prefix(out) +
		cl.build_command(path.generic_string(), obj) +
		ctx.get_response_args().data() +
		cl.get_module_directory(out);
	if (!defines.empty())
		cmd += cl.build_define(defines);
	return [cmd = std::move(cmd)] { return 0 == std::system(cmd.c_str()); };
}

static bool init_build(context& ctx, compiler const& cl) {
	// Bail if no compiler is selected
	if (cl.name.empty()) {
		std::println(std::cerr, "<gbs> No compiler selected/found.");
		return false;
	}

	// Set the default build configuration if not specified
	if (ctx.get_config().empty())
		if (!cmd_config(ctx, cl, "debug,warnings"))
			return false;

	// Set the target operating system in the context
	if (cl.name == "msvc") {
		ctx.set_target_os(operating_system::windows);
	}
	else {
		std::string const base_name = fs::current_path().stem().generic_string();
		auto const cmd = cl.build_command_prefix(ctx.output_dir(cl).generic_string()) + ctx.get_response_args().data() + " -dumpmachine > arch.txt";
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
	fs::create_directories(ctx.output_dir(cl));

#ifdef _MSC_VER
	// Initialize msvc environment
	if (cl.name == "msvc" /*|| cl.unittest_name == "clang"*/) {
		extern bool init_msvc(context const&, compiler const&);
		if (!init_msvc(ctx, cl))
			return false;
	}
#endif

	return true;
}

static void create_module_map_file(context const& ctx, compiler const& cl, module_map const& modmap) {
	std::ofstream modules(ctx.output_dir(cl) / "MODULES");
	for (auto const& [module, path] : modmap) {
		fs::path const bmi = (ctx.output_dir(cl) / path.filename()).replace_extension("pcm");
		modules << std::format("-fmodule-file={}=\"{}\"\n", module, bmi.generic_string());
	}
}

// Create the object list file
static std::pair<fs::path, fs::file_time_type> create_object_file_list(context const& ctx, compiler const& cl, auto const& name, std::ranges::range auto& paths) {
	fs::path objlist_name = (ctx.output_dir(cl) / name).concat("_OBJLIST");
	std::ofstream objlist(objlist_name);
	fs::file_time_type latest_source_time = fs::file_time_type::min();

	for(fs::path const& src : paths) {
		// Don't include the main source file in the object list, as it contains the 'main' function.
		if (src.filename().stem() == name)
			continue;

		latest_source_time = std::max(latest_source_time, fs::last_write_time(src));
		fs::path const obj = get_object_filepath(src, ctx, cl);
		objlist << obj.generic_string() << ' ';
	}
	objlist.close();

	return { objlist_name, latest_source_time };
}

static task_ptr create_build_task(context const& ctx, compiler const& cl, task_graph& tg, fs::path const& prefix, fs::path const& path, module_map& modmap, imports_map& impmap, std::string_view defines = {}) {
	if (!is_valid_sourcefile(path) || !should_include(ctx, path))
		return {};

	source_dependency const deps = extract_module_dependencies(path);
	if (deps.is_export())
		modmap[deps.export_name] = path;
	impmap[path].insert_range(deps.import_names);

	fs::path const obj = get_object_filepath(path, ctx, cl);
	if (is_file_out_of_date(path, obj)) {
		return tg.create_task(make_task_name(prefix, path), make_build_job(ctx, cl, path, obj, defines));
	}
	return {};
}

static task_ptr create_static_lib(context const& ctx, compiler const& cl, task_graph& graph, fs::path const& prefix, fs::path project_dir, fs::path objlist_name, fs::file_time_type latest_source_time) {
	std::string const out_dir = ctx.output_dir(cl).generic_string();
	operating_system const target_os = ctx.get_target_os();

	return graph.create_task(make_task_name(prefix, project_dir), [=] {
		auto const ext_name = project_dir.extension().generic_string().substr(1);

		// static library, create the .lib
		std::string const lib_name = os_get_static_library_name(target_os, ext_name);

		auto const lib_path = fs::path{ out_dir } / lib_name;
		if (fs::exists(lib_path) && latest_source_time < fs::last_write_time(lib_path))
			return true;

		std::string const cmd = cl.static_library_command(lib_name, out_dir) + std::format(" @{}", objlist_name.generic_string());
		std::println("<gbs> Linking static library '{}'...", lib_name);
		return 0 == std::system(cmd.c_str());
		});
}

static task_ptr create_dynamic_lib(context const& ctx, compiler const& cl, task_graph& graph, fs::path const& prefix, fs::path project_dir, fs::path objlist_name, fs::file_time_type latest_source_time) {
	std::string const out_dir = ctx.output_dir(cl).generic_string();
	operating_system const target_os = ctx.get_target_os();

	return graph.create_task(make_task_name(prefix, project_dir), [=] {
		auto const ext_name = project_dir.extension().generic_string().substr(1);

		// dynamic library, link the .dll
		std::string const lib_name = os_get_static_library_name(target_os, ext_name);
		std::string const dll_name = os_get_dynamic_library_name(target_os, ext_name);

		auto const dll_path = fs::path{ out_dir } / dll_name;
		if (fs::exists(dll_path) && latest_source_time < fs::last_write_time(dll_path))
			return true;

		std::string const cmd = cl.dynamic_library_command(dll_name, lib_name, out_dir)
			+ std::format(" @{}", objlist_name.generic_string()) + cl.create_link_libraries();
		std::println("<gbs> Linking dynamic library '{}'...", dll_name);
		return 0 == std::system(cmd.c_str());
		});
}

static task_ptr create_executeable(context const& ctx, compiler const& cl, task_graph& graph, fs::path const& prefix, std::string name, fs::path project_dir, fs::path objlist_name, fs::file_time_type latest_source_time, fs::path extra_objlist_name = {}) {
	std::string const out_dir = ctx.output_dir(cl).generic_string();
	operating_system const target_os = ctx.get_target_os();

	return graph.create_task(make_task_name(prefix, project_dir), [=] {
		std::string const main_obj_name = (fs::path{ out_dir } / name).replace_extension("obj").generic_string();
		std::string const exe_name = os_get_executable_name(target_os, name);

		auto const exe_path = fs::path{ out_dir } / exe_name;
		if (fs::exists(exe_path) && latest_source_time < fs::last_write_time(exe_path))
			return true;

		std::println("<gbs> Linking executable '{}'...", exe_name);
		std::string obj_resp = std::format(" @{}", objlist_name.generic_string());
		if (!extra_objlist_name.empty())
			obj_resp += std::format(" @{}", extra_objlist_name.generic_string());
		obj_resp += std::format(" {}", main_obj_name);
		std::string const cmd = cl.create_link_command(exe_name, out_dir) + obj_resp + cl.create_link_library_list(out_dir) + cl.create_link_libraries();
		return 0 == std::system(cmd.c_str());
		});
}

static bool add_build_tasks(context& ctx, compiler const& cl, task_graph& graph, fs::path const& prefix) {
	std::set<fs::path> libs;
	module_map modmap;
	imports_map impmap;
	task_map tasks;

	auto project_it = directories
		| transform([](std::string_view p) { return fs::directory_iterator(p, fs::directory_options::follow_directory_symlink); })
		| join
		| filter([](fs::directory_entry const& e) { return e.is_directory(); })
		| transform(&fs::directory_entry::path)
		| transform(&fs::path::lexically_normal)
		| filter(is_valid_project_name)
		| filter(std::bind_front(&should_include, std::cref(ctx)));
	auto include_dirs_it = fs::recursive_directory_iterator(".", fs::directory_options::follow_directory_symlink)
		| filter([](fs::directory_entry const& e) { return e.is_directory(); })
		| transform(&fs::directory_entry::path)
		| transform(&fs::path::lexically_normal)
		| filter([](fs::path const& p) { return p.filename() == "inc" || p.filename() == "include"; })
		| transform([&cl](fs::path const& p) { return cl.make_include_path(p); });

	// Add the std module to the build
	if (cl.std_module) {
		if (task_ptr const std_module_task = create_build_task(ctx, cl, graph, prefix, *cl.std_module, modmap, impmap))
			tasks[*cl.std_module] = std_module_task;
	}
	else {
		std::println("<gbs> warning: '{}' does not provide a standard library module, builds may fail if the standard library is used", cl.name_and_version);
	}

	// Create the barrier task for the 'lib' directory.
	// Library projects will complete before this task, and all other projects will depend on this task.
	task_ptr lib_barrier_task = graph.create_task(make_task_name(prefix, "lib"), task_true);

	// Find all project directories and create build tasks for them
	for (fs::path project_dir : project_it) {
		bool const is_lib_path = *project_dir.begin() == "lib";
		fs::path const project_name = *std::next(project_dir.begin(), is_lib_path); // get_project_name

		// Create the object list file for the .lib file
		auto source_files = get_source_files(ctx, cl, project_dir / "src") | std::ranges::to<std::vector>();
		auto const [objlist_name, latest_source_time] = create_object_file_list(ctx, cl, project_name, source_files);

		std::string const lib_base_name = is_lib_path && project_name.has_extension()
			? project_name.extension().generic_string().substr(1)
			: project_name.generic_string();

		task_ptr project_link_task;
		// TODO merge
		switch (get_project_type(project_dir)) {
		case project_type::static_lib:
			project_link_task = create_static_lib(ctx, cl, graph, prefix, project_dir, objlist_name, latest_source_time);
			libs.insert(ctx.output_dir(cl) / os_get_static_library_name(ctx.get_target_os(), lib_base_name));
			break;
		case project_type::dynamic_lib:
			project_link_task = create_dynamic_lib(ctx, cl, graph, prefix, project_dir, objlist_name, latest_source_time);
			{
				auto const lib_or_dll_name = ctx.get_target_os() != operating_system::windows
					? os_get_dynamic_library_name(ctx.get_target_os(), lib_base_name)
					: os_get_static_library_name(ctx.get_target_os(), lib_base_name);
				libs.insert(ctx.output_dir(cl) / lib_or_dll_name);
			}
			break;
		case project_type::executable:
			project_link_task = create_executeable(ctx, cl, graph, prefix, project_name.generic_string(), project_dir, objlist_name, latest_source_time);
			break;
		case project_type::unknown:
			// Stuff like headers-only libraries, etc. that don't have a 'src' directory
			break;
		}

		if (is_lib_path) {
			// This project must complete before the lib barrier
			graph.add_dependency(project_link_task, lib_barrier_task);
		}
		else {
			// This project can only complete after the lib barrier
			graph.add_dependency(lib_barrier_task, project_link_task);
		}

		// Save library object paths
		std::string export_define;
		if (is_lib_path && project_name.has_extension() && project_name.stem() == "d")
			export_define = to_upper(project_name.extension().generic_string().substr(1)) + "_EXPORTS";

		// Find all source files and create build tasks for them
		for (fs::path source_path : source_files) {
			task_ptr task = create_build_task(ctx, cl, graph, prefix, source_path, modmap, impmap, export_define);
			tasks[source_path] = task;
			graph.add_dependency(task, project_link_task);
		}

		// Create tests
		if (fs::exists(project_dir / "tests")) {
			auto tests_files = get_source_files(ctx, cl, project_dir / "tests") | std::ranges::to<std::vector>();
			auto const [tests_objlist_name, _] = create_object_file_list(ctx, cl, "tests", tests_files);

			std::string const test_name = "tests." + project_name.generic_string();
			task_ptr tests_task = create_executeable(ctx, cl, graph, prefix, test_name, project_dir, objlist_name, latest_source_time, tests_objlist_name);
			ctx.add_unittest(ctx.output_dir(cl) / os_get_executable_name(ctx.get_target_os(), test_name), cl.wsl);

			for (fs::path source_path : tests_files) {
				task_ptr task = create_build_task(ctx, cl, graph, prefix, source_path, modmap, impmap);
				tasks[source_path] = task;
				graph.add_dependency(task, tests_task);
			}
		}
	}

	// Set up the task dependencies for modules
	for (auto const& [path, impset] : impmap) {
		if (task_ptr const& task = graph.find_task(make_task_name(prefix, path)); task) {
			for (auto const& imp : impset) {
				if (modmap.contains(imp)) {
					fs::path const& dep_path = modmap.at(imp);
					task_ptr const& dep_task = graph.find_task(make_task_name(prefix, dep_path));
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
	create_module_map_file(ctx, cl, modmap);
	create_response_file(ctx, cl, "LIBLIST", libs);
	create_response_file(ctx, cl, "SRC_INCLUDES", include_dirs_it);

	return true;
}

bool cmd_build(context& ctx, std::string_view /*target*/) {
	std::println("<gbs> Building (v2) ...");

	std::vector<compiler> const compilers = ctx.get_selected_compilers();
	if (compilers.empty()) {
		std::println(std::cerr, "<gbs> No compiler selected/found.");
		return false;
	}

	if (compilers.size() > 1)
		std::println("<gbs> Building with {} compilers...", compilers.size());

	std::string const config = ctx.get_config().empty()
		? std::string{ "debug,warnings" }
		: std::string{ ctx.get_config() };

	task_graph graph;

	for (compiler const& cl : compilers) {
		if (!cmd_config(ctx, cl, config))
			return false;

		if (!init_build(ctx, cl))
			return false;

		if (!add_build_tasks(ctx, cl, graph, compiler_task_prefix(cl)))
			return false;
	}

	graph.run();

	return !graph.was_aborted();
}
