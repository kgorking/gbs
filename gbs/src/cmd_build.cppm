export module cmd_build;
import std;
import env;
import context;
import get_source_groups;
import monad;
import cmd_config;

namespace fs = std::filesystem;

static bool is_file_out_of_date(fs::path const& path, std::set<std::string> const&, fs::path const& obj) {
	if (!fs::exists(obj))
		return true;

	return (fs::last_write_time(obj) <= fs::last_write_time(path));
}

// Why doesn't this garbage stl have this already???
static std::string to_upper(std::string const& cstr) {
	std::string str = cstr;
	std::use_facet<std::ctype<char>>(std::locale()).toupper(str.data(), str.data() + str.size());
	return str;
}

static bool should_not_exclude(fs::path const& path) {
	return	!path.string().starts_with("x.") &&
			!path.filename().stem().string().starts_with("x.");
}

static auto get_object_filepath(fs::path const& path, std::set<std::string> const& imports, context const& ctx) {
	fs::path const obj = (ctx.output_dir() / path.filename()).replace_extension("obj");
	return std::tuple{ path, imports, obj };
}

/*static void store_object_filepath([[maybe_unused]] fs::path const& path, [[maybe_unused]] std::set<std::string> const& imports, fs::path const& obj, std::shared_mutex& mut, std::set<fs::path>& objects) {
	if (path.parent_path().parent_path().stem() == "d")
		return;
	std::scoped_lock sl(mut);
	objects.insert(obj);
}*/

static std::string make_build_command(fs::path const& path, std::set<std::string> const& imports, fs::path const& obj, std::unordered_map<fs::path, std::string> const& path_defines, context const& ctx) {
	fs::path const parent_dir = path.parent_path();
	std::string const define = path_defines.contains(parent_dir) ? ctx.build_define(path_defines.at(parent_dir)) : "";

	return ctx.build_command_prefix() +
		ctx.build_command(path.string(), obj) +
		ctx.get_response_args().data() +
		ctx.build_references(imports) +
		define;
}

// The build command
export bool cmd_build(context& ctx, std::string_view /*const args*/) {
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


	std::println("<gbs> Building...");

	// Make sure the output and response directories exist
	fs::create_directories(ctx.output_dir());

#ifdef _MSC_VER
	// Initialize msvc environment
	if (selected_cl.name == "msvc" || selected_cl.name == "clang") {
		extern bool init_msvc(context const&);
		if (!init_msvc(ctx))
			return false;
	}
#endif

	// Save paths to libraries and objects
	std::set<fs::path> libs;
	std::set<fs::path> objects;
	std::shared_mutex mut_libs;
	//std::shared_mutex mut_objs;

	// Containers for all source files, includes, defines and targets
	depth_ordered_sources_map all_sources;
	std::unordered_set<fs::path> includes;
	std::unordered_map<fs::path, std::string> dlib_defines;
	std::unordered_map<fs::path, std::vector<fs::path>> executables;
	std::unordered_map<fs::path, std::vector<fs::path>> dynamic_libraries;
	std::unordered_map<fs::path, std::vector<fs::path>> unittests;

	// If a standard module is specified, add it to the source- and object list
	if (ctx.get_selected_compiler().std_module) {
		all_sources[0][*ctx.get_selected_compiler().std_module] = std::set<std::string>{};
		objects.insert((ctx.output_dir() / ctx.get_selected_compiler().std_module->filename()).replace_extension("obj"));
	}

	// Find all source files
	as_monad({ "lib", "." })
		.filter([](auto const& p) { return fs::exists(p); })
		.as<fs::directory_iterator>(fs::directory_options::follow_directory_symlink | fs::directory_options::skip_permission_denied)
		.join()
		.filter(&fs::directory_entry::is_directory)
		.map(&fs::directory_entry::path)
		.map(&fs::path::lexically_normal)
		.filter([](fs::path const& p) { return should_not_exclude(p) && fs::exists(p / "src"); })
		.then([&](fs::path const& p) {
			// Get the source files
			auto const source_files = get_grouped_source_files(p / "src");
			auto const files_view = source_files | std::views::values | std::views::join | std::views::keys;

			if ("lib" == *p.begin()) {
				if (fs::exists(p / "src")) includes.insert(p / "src");
				if (fs::exists(p / "inc")) includes.insert(p / "inc");
				if (fs::exists(p / "include")) includes.insert(p / "include");

				if (p.has_extension() && p.stem() == "s") {
					for (fs::path const& path : files_view) {
						if(should_not_exclude(path))
							objects.insert((ctx.output_dir() / path.filename()).replace_extension("obj"));
					}
				}
				else if (p.has_extension() && p.stem() == "d") {
					dynamic_libraries[p].append_range(files_view);
					dlib_defines[p / "src"] = to_upper(p.extension().generic_string().substr(1)) + "_EXPORTS";
				}
			}
			else if ("unittest" == *p.begin()) {
				unittests[p].append_range(files_view);
			}
			else {
				executables[p].append_range(files_view);
			}

			for (auto [index, sources] : source_files)
				all_sources[index].merge(sources);
		});

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

	// Compile all source files
	if (!as_monad(all_sources)
			.join()
			.values()
			.join_par()
			.filter([](fs::path const& path, auto const&) { return should_not_exclude(path); })
			.map(get_object_filepath, ctx)
			.filter(is_file_out_of_date)
			.map(make_build_command, dlib_defines, ctx)
			.until([](std::string_view const cmd) noexcept { return (0 == std::system(cmd.data())); }))
		return false; // Bail if compilation failed


	// Link dynamic libraries
	bool ok = true;
	std::for_each(std::execution::par_unseq, dynamic_libraries.begin(), dynamic_libraries.end(), [&](std::pair<fs::path const, std::vector<fs::path>> const& pair) {
		if (!ok) return;

		auto const& [p, vec] = pair;
		std::string const name = p.extension().string().substr(1);

		// Create the object list file for the .lib file
		fs::path objlist_name = name + "_OBJLIST";
		std::ofstream dll_objlist(ctx.output_dir() / objlist_name);
		for (fs::path const& src : vec) {
			fs::path const obj = (ctx.output_dir() / src.filename()).replace_extension("obj");
			dll_objlist << obj.generic_string() << ' ';
		}
		dll_objlist.close();

		std::string const cmd = ctx.dynamic_library_command(name, ctx.output_dir().generic_string()) + std::format(" @{}/{}", ctx.output_dir().generic_string(), objlist_name.generic_string());

		{
			std::scoped_lock sl(mut_libs);
			libs.insert(ctx.output_dir() / (name + ".lib"));
		}

		std::println("<gbs> Creating dynamic library '{}'...", name);
		ok = (0 == std::system(cmd.c_str()));
		});

	if (!ok)
		return false;

	// Create the library list file
	std::ofstream liblist(ctx.output_dir() / "LIBLIST");
	for (fs::path const& lib : libs)
		liblist << lib.generic_string() << ' ';
	liblist.close();

	// Link unittests
	std::for_each(std::execution::par_unseq, unittests.begin(), unittests.end(), [&](std::pair<fs::path const, std::vector<fs::path>>& pair) {
		if (!ok) return;

		auto& [p, vec] = pair;

		std::string const name = p == "." ? fs::current_path().stem().string() : p.stem().string();

		// Partion the source files into unittests and support files
		auto it = std::ranges::partition(vec, [](fs::path const& path) { return !path.filename().string().starts_with("test."); });

		// Create the object list file for non-test files
		fs::path objlist_name = name + "_OBJLIST";
		std::ofstream exe_objlist(ctx.output_dir() / objlist_name);
		std::for_each(vec.begin(), std::begin(it), [&](fs::path const& src) {
			fs::path const obj = (ctx.output_dir() / src.filename()).replace_extension("obj");
			exe_objlist << obj.generic_string() << ' ';
			});
		exe_objlist.close();

		// Link each unittest
		std::ranges::for_each(it, [&](fs::path const& src) {
			if (!ok) return;
			std::string const test_name = src.stem().string();

			std::println("<gbs> Linking unittest '{}'...", test_name);
			std::string const obj_resp = std::format(" @{0}/{1} {0}/{2}.obj", ctx.output_dir().generic_string(), objlist_name.generic_string(), test_name);
			std::string const cmd = ctx.link_command(test_name, ctx.output_dir().generic_string()) + obj_resp;
			ok = (0 == std::system(cmd.c_str()));
			});
		});

	if (!ok)
		return false;

	// Link executables
	std::for_each(std::execution::par_unseq, executables.begin(), executables.end(), [&](std::pair<fs::path const, std::vector<fs::path>> const& pair) {
		if (!ok) return;

		auto const& [p, vec] = pair;

		std::string const name = p == "." ? fs::current_path().stem().string() : p.stem().string();

		// Create the object list file
		fs::path objlist_name = name + "_OBJLIST";
		std::ofstream exe_objlist(ctx.output_dir() / objlist_name);
		for (fs::path const& src : vec) {
			fs::path const obj = (ctx.output_dir() / src.filename()).replace_extension("obj");
			exe_objlist << obj.generic_string() << ' ';
		}
		exe_objlist.close();

		std::println("<gbs> Linking executable '{}'...", name);
		std::string const obj_resp = std::format(" @{0}/{1}", ctx.output_dir().generic_string(), objlist_name.generic_string());
		std::string const cmd = ctx.link_command(name, ctx.output_dir().generic_string()) + obj_resp;
		ok = (0 == std::system(cmd.c_str()));
		});

	return ok;
}
