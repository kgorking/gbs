export module cmd_build;
import std;
import env;
import context;
import response;
import get_source_groups;
import monad;

namespace fs = std::filesystem;
using namespace std::string_view_literals;

// Converts arguments into response files
std::string convert_arg_to_response(std::string_view arg, fs::path response_dir) {
	return std::format(" @{}/{}", response_dir.string(), arg);
}

// Gets all the source files and adds the STL module
depth_ordered_sources_map get_all_source_files(std::string_view path, context const& ctx) {
	auto result = get_grouped_source_files(path);
	if (ctx.selected_cl.std_module) {
		result[0].emplace_back(*ctx.selected_cl.std_module, std::set<std::string>{});
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

std::string make_build_command(std::tuple<fs::path, std::set<std::string>, std::string> const& in, context const& ctx, std::string_view cmd_prefix, std::string_view resp_args) {
	auto const& [path, imports, obj] = in;

	return 
		std::string{ cmd_prefix } +
		ctx.build_command(path.string(), obj) +
		resp_args.data() +
		ctx.build_references(imports);
}

// The build command
export bool cmd_build(context& ctx, std::string_view args) {
	// Bail if no compiler is selected
	if (ctx.selected_cl.name.empty()) {
		std::println(std::cerr, "<gbs> No compiler selected/found.");
		return false;
	}

	// Ensure the current working directory is valid
	if (!fs::exists("src/")) {
		std::println("<gbs> Error: no 'src' directory found at '{}'", fs::current_path().generic_string());
		return false;
	}

	// Print the compiler version
	std::println(std::cerr, "<gbs> Building with '{} {}.{}.{}'", ctx.selected_cl.name, ctx.selected_cl.major, ctx.selected_cl.minor, ctx.selected_cl.patch);

	// Set the default build config if none is specified
	if (args.empty())
		args = "release,warnings";

	// Ensure the needed response files are present
	init_response_files(ctx);
	check_response_files(ctx, args);

	// Get the build output directory
	// and create the build dirs if needed
	ctx.config = args.substr(0, args.find(','));
	auto const output_dir = ctx.output_dir();
	fs::create_directories(output_dir);

	// Arguments to the compiler.
	std::string const resp_args = as_monad(args)
			.join()
			.split(',')
			.prefix("_shared"sv)
			.map(convert_arg_to_response, ctx.response_dir())
			.to<std::string>();

#ifdef _MSC_VER
	if (ctx.selected_cl.name == "msvc" || ctx.selected_cl.name == "clang") {
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
		.map(make_build_command, ctx, ctx.build_command_prefix(), resp_args)
		.until(+[](std::string_view cmd) noexcept {
			return (0 == std::system(cmd.data()));
		});

	// Close the objects file
	objects.close();

	// Link sources
	if (succeeded) {
		std::println("<gbs> Linking...");
		std::string const executable = fs::current_path().stem().string();
		std::string const link = ctx.link_command(executable);
		return 0 == std::system(link.c_str());
	}

	return false;
}
