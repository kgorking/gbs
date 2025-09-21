export module cmd_build;
import std;
import env;
import context;
import get_source_groups;
import monad;
import cmd_config;

namespace fs = std::filesystem;
using namespace std::string_view_literals;

bool is_file_up_to_date(std::filesystem::path const& in, std::filesystem::path const& out) {
	if (!std::filesystem::exists(out))
		return false;

	return (std::filesystem::last_write_time(out) > std::filesystem::last_write_time(in));
}

// Gets all the source files and adds the STL module
depth_ordered_sources_map get_all_source_files(std::string_view const path, context const& ctx) {
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
	auto const& selected_cl = ctx.get_selected_compiler();

	// Bail if no compiler is selected
	if (selected_cl.name.empty()) {
		std::println(std::cerr, "<gbs> No compiler selected/found.");
		return false;
	}

	// Set the default build configuration if not specified
	if (ctx.get_config().empty())
		if (!cmd_config(ctx, "debug,warnings"))
			return false;

	// Ensure the current working directory is valid
	if (!fs::exists("src/")) {
		std::println("<gbs> Error: no 'src' directory found at '{}'", fs::current_path().generic_string());
		return false;
	}

	// Print the compiler version
	std::println(std::cerr, "<gbs> Building with '{} {}.{}.{}'", selected_cl.name, selected_cl.major, selected_cl.minor, selected_cl.patch);

	auto const output_dir = ctx.output_dir();

#ifdef _MSC_VER
	if (selected_cl.name == "msvc" || selected_cl.name == "clang") {
		extern bool init_msvc(context const&);
		if (!init_msvc(ctx))
			return false;
	}
#endif

	// Create file containing the list of objects to link
	std::ofstream objects(output_dir / "OBJLIST");
	std::shared_mutex mut;

	// Get the source files and compile them.
	bool const succeeded = as_monad(get_all_source_files("src", ctx))
		.join()
		.values()
		.join_par()
		.guard([](std::exception const& e) { std::println(std::cerr, "<gbs> Error: {}", e.what()); })
		.map(write_object_file_and_check_date, ctx, std::ref(mut), std::ref(objects))
		.map(make_build_command, ctx)
		.until(+[](std::string_view cmd) noexcept {
			return (0 == std::system(cmd.data()));
		});

	if (!succeeded)
		return false;

	// Close the objects file
	objects.close();

	// Link sources
	std::println("<gbs> Linking...");
	std::string const executable = fs::current_path().stem().string();
	std::string const link = ctx.link_command(executable);
	return 0 == std::system(link.c_str());
}
