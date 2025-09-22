export module cmd_build;
import std;
import env;
import context;
import get_source_groups;
import monad;
import cmd_config;

namespace fs = std::filesystem;

bool is_file_up_to_date(fs::path const& in, fs::path const& out) {
	if (!fs::exists(out))
		return false;

	return (fs::last_write_time(out) > fs::last_write_time(in));
}

// Gets all the source files and adds the STL module
depth_ordered_sources_map get_all_source_files(fs::path const& path, context const& ctx) {
	auto result = get_grouped_source_files(path);
	if (ctx.get_selected_compiler().std_module) {
		result[0].emplace_back(*ctx.get_selected_compiler().std_module, std::set<std::string>{});
	}
	return result;
}

auto write_object_file_and_check_date(source_info si, context const& ctx, std::shared_mutex& mut, std::ofstream& objects) -> std::optional<std::tuple<fs::path, std::set<std::string>, std::string>> {
	fs::path const obj = (ctx.output_dir() / si.first.filename()).replace_extension("obj");

	{
		std::scoped_lock sl(mut);
		objects << obj << ' ';
	}

	if (!is_file_up_to_date(si.first, obj))
		return std::tuple{ std::move(si.first), std::move(si.second), obj.string() };
	else
		return {};
}

std::string make_build_command(std::tuple<fs::path, std::set<std::string>, std::string> const& in, context const& ctx) {
	auto const& [path, imports, obj] = in;

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

	// Set up the library input files
	std::ofstream libs(ctx.output_dir() / "LIBLIST");

	// Gets all the directories to compile
	constexpr auto dir_search_options = fs::directory_options::follow_directory_symlink | fs::directory_options::skip_permission_denied;

	return as_monad({ "lib", "unittest" })
		.filter([](auto const& p) { return fs::exists(p); })
		.as<fs::directory_iterator>(dir_search_options)
			.join()
			.filter(&fs::directory_entry::is_directory)
			.filter([](fs::path const& p) { return fs::exists(p / "src"); })
			.map([](fs::path const& p) { return fs::absolute(p); })
			.concat(fs::current_path())
			.until([&](fs::path const& p) {
				std::println("<gbs> compiling '{}/src'", p.generic_string());

				// Create file containing the list of objects to link
				auto const output_dir = ctx.output_dir();
				std::ofstream objects(output_dir / "OBJLIST");
				std::shared_mutex mut;

				// Get the source files and compile them.
				bool const succeeded = as_monad(get_all_source_files(p / "src", ctx))
					.join()
					.values()
					.join_par()
					.guard([](std::exception const& e) { std::println(std::cerr, "<gbs> Exception: {}", e.what()); })
					.map(write_object_file_and_check_date, ctx, std::ref(mut), std::ref(objects))
					.map(make_build_command, ctx)
					.until([](std::string_view const cmd) noexcept { return (0 == std::system(cmd.data())); });

				// Close the objects file
				objects.close();

				// Bail if compilation failed
				if (!succeeded)
					return false;

				// Link/lib sources
				if (p.stem() == "s") {
					std::string const name = p.extension().string().substr(1);
					std::string const cmd = ctx.static_library_command(name, output_dir.generic_string());

					libs << (output_dir / (name + ".lib")) << ' ';

					std::println("<gbs> Creating static library '{}'...", name);
					return 0 == std::system(cmd.c_str());
				}
				else if (p.stem() == "d") {
					std::string const name = p.extension().string().substr(1);
					std::string const cmd = ctx.dynamic_library_command(name, output_dir.generic_string());

					libs << (output_dir / (name + ".lib")) << ' ';

					std::println("<gbs> Creating dynamic library '{}'...", name);
					return 0 == std::system(cmd.c_str());
				}
				else {
					libs.close();

					std::string const name = p.stem().string();
					std::string const cmd = ctx.link_command(name, output_dir.generic_string()) + std::format(" @{}/LIBLIST", output_dir.generic_string());

					std::println("<gbs> Linking executable '{}'...", name);
					return 0 == std::system(cmd.c_str());
				}
			});
}
