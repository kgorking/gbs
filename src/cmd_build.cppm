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

// Gets all the source files and adds the STL module
depth_ordered_sources_map get_all_source_files(fs::path const& path, context const& ctx) {
	auto result = get_grouped_source_files(path);
	if (ctx.get_selected_compiler().std_module) {
		result[0][*ctx.get_selected_compiler().std_module] = std::set<std::string>{};
	}
	return result;
}

auto get_object_filepath(fs::path const& path, std::set<std::string> const& imports, context const& ctx) {
	fs::path const obj = (ctx.output_dir() / path.filename()).replace_extension("obj");
	return std::tuple{ path, imports, obj };
}

void store_object_filepath([[maybe_unused]] fs::path const& path, [[maybe_unused]] std::set<std::string> const& imports, fs::path const& obj, std::shared_mutex& mut, std::set<fs::path>& objects) {
	std::scoped_lock sl(mut);
	objects.insert(obj);
}

std::string make_build_command(fs::path const& path, std::set<std::string> const& imports, fs::path const& obj, context const& ctx) {
	//auto const& [path, imports, obj] = in;

	return ctx.build_command_prefix() +
		ctx.build_command(path.string(), obj) +
		ctx.get_response_args().data() +
		ctx.build_references(imports);
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

	// TODO
	// Find all source files. Recursively detect all subfolders that need compilation. A 'lib' subfolder might have its own 'lib' or 'unittest' subfolders.
	depth_ordered_sources_map all_sources; // .merge
	// Each subfolder goes in a list according to its name.
	std::vector<fs::path> dynamic_libraries;
	std::vector<fs::path> executables;
	// All source files are compiled in parallel.
	// All dynamic library projects are linked in parallel.
	// Finally, all executables are linked in parallel.

	auto const output_dir = ctx.output_dir();

	std::println("<gbs> Building...");
	as_monad({ "lib", "unittest" })
		.filter([](auto const& p) { return fs::exists(p); })
		.as<fs::directory_iterator>(dir_search_options)
			.join()
			.filter(&fs::directory_entry::is_directory)
			.filter([](fs::path const& p) { return fs::exists(p / "src"); })
			.map([](fs::path const& p) { return fs::absolute(p); })
			.concat(fs::current_path())
			.then([&](fs::path const& p) {

				// Get the source files and store them.
				for (auto [index, sources] : get_all_source_files(p / "src", ctx))
					all_sources[index].merge(sources);

				// Link/lib sources
				if (p.stem() == "s") {
					// do nothing, static libs are a scam
				}
				else if (p.stem() == "d") {
					dynamic_libraries.push_back(p);
				}
				else {
					executables.push_back(p);
				}
			});

	if (!as_monad(all_sources)
			.join()
			.values()
			.join_par()
			.map(get_object_filepath, ctx)
			.and_then(store_object_filepath, std::ref(mut_objs), std::ref(objects))
			.filter(is_file_out_of_date)
			.map(make_build_command, ctx)
			.until([](std::string_view const cmd) noexcept { return (0 == std::system(cmd.data())); }))
		return false; // Bail if compilation failed

	// Create the object list file
	std::ofstream objlist(ctx.output_dir() / "OBJLIST");
	for (fs::path const& obj : objects)
		objlist << obj << ' ';
	objlist.close();

	bool ok = true;
	std::for_each(std::execution::par_unseq, dynamic_libraries.begin(), dynamic_libraries.end(), [&](fs::path const& p) {
		if (!ok) return;

		std::string const name = p.extension().string().substr(1);
		std::string const cmd = ctx.dynamic_library_command(name, output_dir.generic_string());

		{
			//std::scoped_lock sl(mut_libs);
			//libs.insert(output_dir / (name + ".lib"));
		}

		//std::println("<gbs> Creating dynamic library '{}'...", name);
		ok = (0 == std::system(cmd.c_str()));
		});

	// Create the library list file
	//std::ofstream liblist(ctx.output_dir() / "LIBLIST");
	//for (fs::path const& lib : libs)
	//	liblist << lib << ' ';
	//liblist.close();

	if (!ok)
		return false;

	std::for_each(std::execution::par_unseq, executables.begin(), executables.end(), [&](fs::path const& p) {
		if (!ok) return;

		std::string const name = p.stem().string();
		std::string const cmd = ctx.link_command(name, output_dir.generic_string());// +std::format(" @{}/LIBLIST", output_dir.generic_string());

		//std::println("<gbs> Linking executable '{}'...", name);
		ok = (0 == std::system(cmd.c_str()));
		});

	return ok;
}
