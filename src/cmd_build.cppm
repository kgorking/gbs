export module cmd_build;
import std;
import env;
import context;
import get_source_groups;
import monad;
import cmd_config;

namespace fs = std::filesystem;

bool is_file_out_of_date(fs::path const& path, std::set<std::string> const&, fs::path const& obj) {
	if (!fs::exists(obj))
		return true;

	return (fs::last_write_time(obj) <= fs::last_write_time(path));
}

// Why doesn't this garbage stl have this already???
std::string to_upper(std::string const& cstr) {
	std::string str = cstr;
	std::use_facet<std::ctype<char>>(std::locale()).toupper(str.data(), str.data() + str.size());
	return str;
}

// Gets all the source files and adds the STL module
depth_ordered_sources_map get_all_source_files(fs::path const& path, context const& ctx) {
	auto result = get_grouped_source_files(path);
	if (ctx.get_selected_compiler().std_module) {
		result[0][*ctx.get_selected_compiler().std_module] = std::set<std::string>{};
	}
	return result;
}

bool should_not_exclude(fs::path const& path, std::set<std::string> const& /*imports*/) {
	return !path.filename().stem().string().starts_with("x.");
}

auto get_object_filepath(fs::path const& path, std::set<std::string> const& imports, context const& ctx) {
	fs::path const obj = (ctx.output_dir() / path.filename()).replace_extension("obj");
	return std::tuple{ path, imports, obj };
}

void store_object_filepath([[maybe_unused]] fs::path const& path, [[maybe_unused]] std::set<std::string> const& imports, fs::path const& obj, std::shared_mutex& mut, std::set<fs::path>& objects) {
	if (path.parent_path().parent_path().stem() == "d")
		return;
	std::scoped_lock sl(mut);
	objects.insert(obj);
}

std::string make_build_command(fs::path const& path, std::set<std::string> const& imports, fs::path const& obj, std::unordered_map<fs::path, std::string> const& path_defines, context const& ctx) {
	auto const parent_dir = path.parent_path();
	std::string const define = path_defines.contains(parent_dir) ? ctx.build_define(path_defines.at(parent_dir)) : "";

	return ctx.build_command_prefix() +
		ctx.build_command(path.string(), obj) +
		ctx.get_response_args().data() +
		ctx.build_references(imports) +
		define;
}

// The build command
export bool cmd_build(context& ctx, std::string_view /*const args*/) {
	// Ensure the current working directory is valid
	if (!fs::exists("src/")) {
		std::println("<gbs> Error: no 'src' directory found at '{}'", fs::current_path().generic_string());
		return false;
	}

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
	std::shared_mutex mut_objs;

	// Gets all the directories to compile
	constexpr auto dir_search_options = fs::directory_options::follow_directory_symlink | fs::directory_options::skip_permission_denied;

	// Find all source files. Recursively detect all subfolders that need compilation. A 'lib' subfolder might have its own 'lib' or 'unittest' subfolders.
	depth_ordered_sources_map all_sources;
	std::unordered_map<fs::path, std::vector<fs::path>> dynamic_libraries;
	std::unordered_set<fs::path> includes;
	std::vector<fs::path> executables;
	std::unordered_map<fs::path, std::string> dlib_defines;

	if (ctx.get_selected_compiler().std_module) {
		all_sources[0][*ctx.get_selected_compiler().std_module] = std::set<std::string>{};
	}

	auto const output_dir = ctx.output_dir();

	std::println("<gbs> Building...");
	as_monad({ "lib", "unittest/lib" })
		.filter([](auto const& p) { return fs::exists(p); })
		.as<fs::directory_iterator>(dir_search_options)
		.join()
		.filter(&fs::directory_entry::is_directory)
		.filter([](fs::path const& p) { return fs::exists(p / "src"); })
		.concat(fs::path("."))
		.then([&](fs::path const& p) {
			includes.insert(p / "src");
			if (fs::exists(p / "inc")) includes.insert(p / "inc");
			if (fs::exists(p / "include")) includes.insert(p / "include");

			// Get the source files and store them.
			auto const source_files = get_grouped_source_files(p.lexically_normal() / "src");
			for (auto [index, sources] : source_files) {
				as_monad(sources).join().keys().map(&fs::path::parent_path).to_dest(includes);
				all_sources[index].merge(sources);
			}

			if (p.has_extension() && p.stem() == "s") {
				// do nothing, static libs are a scam
			}
			else if (p.has_extension() && p.stem() == "d") {
				std::vector<fs::path>& vec = dynamic_libraries[p];
				for (const auto& source_group : source_files | std::views::values)
					for (const auto& source_file : source_group | std::views::keys)
						vec.push_back(source_file.generic_string());
				dlib_defines[p / "src"] = to_upper(p.extension().generic_string().substr(1)) + "_EXPORTS";
			}
			else {
				executables.push_back(p);
			}
		});

	// Create a response file for all include paths
	{
		std::ofstream includes_rsp(ctx.output_dir() / "SRC_INCLUDES");
		for (fs::path const& include : includes)
			includes_rsp << ctx.make_include_path(include.generic_string()) << ' ';
		includes_rsp.close();
	}

	if (!as_monad(all_sources)
			.join()
			.values()
			.join_par()
			.filter(should_not_exclude)
			.map(get_object_filepath, ctx)
			.and_then(store_object_filepath, std::ref(mut_objs), std::ref(objects))
			.filter(is_file_out_of_date)
			.map(make_build_command, dlib_defines, ctx)
			.until([](std::string_view const cmd) noexcept { return (0 == std::system(cmd.data())); }))
		return false; // Bail if compilation failed

	// Create the object list file
	std::ofstream objlist(ctx.output_dir() / "OBJLIST");
	for (fs::path const& obj : objects)
		objlist << obj.generic_string() << ' ';
	objlist.close();

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

		std::string const cmd = ctx.dynamic_library_command(name, output_dir.generic_string()) + std::format(" @{}/{}", output_dir.generic_string(), objlist_name.generic_string());

		{
			std::scoped_lock sl(mut_libs);
			libs.insert(output_dir / (name + ".lib"));
		}

		std::println("<gbs> Creating dynamic library '{}'...", name);
		ok = (0 == std::system(cmd.c_str()));
		});

	// Create the library list file
	std::ofstream liblist(ctx.output_dir() / "LIBLIST");
	for (fs::path const& lib : libs)
		liblist << lib.generic_string() << ' ';
	liblist.close();

	if (!ok)
		return false;

	std::for_each(std::execution::par_unseq, executables.begin(), executables.end(), [&](fs::path const& p) {
		if (!ok) return;

		std::string const name = p == "." ? fs::current_path().stem().string() : p.stem().string();
		std::string const cmd = ctx.link_command(name, output_dir.generic_string());

		std::println("<gbs> Linking executable '{}'...", name);
		ok = (0 == std::system(cmd.c_str()));
		});

	return ok;
}
