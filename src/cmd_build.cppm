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
	//return " @" + (ctx.response_dir() / std::string{ arg.begin(), arg.end() }).string();
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
	ctx.config = args.substr(0, args.find(','));
	auto const output_dir = ctx.output_dir();

	// Create the build dirs if needed
	fs::create_directories(ctx.output_dir());

	// Arguments to the compiler.
	// TODO std::views::concat("_shared", args)
	std::string const resp_args = convert_arg_to_response("_shared"sv, ctx.response_dir())
		+ as_monad(args)
			.iter()
			.split(',')
			.map(convert_arg_to_response, ctx.response_dir())
			.join()
			.to<std::string>();

#ifdef _MSC_VER
	if (ctx.selected_cl.name == "msvc") {
		extern bool init_msvc(context const&);
		if (!init_msvc(ctx))
			return false;
	}
	else
#endif
		if (ctx.selected_cl.name.starts_with("clang")) {
	}
	else if (ctx.selected_cl.name == "gcc") {
	}
	else {
		std::println("<gbs> INTERNAL : unknown compiler {:?}", ctx.selected_cl.name);
		return false;
	}

	// Get the source files to compile
	auto sources = get_grouped_source_files("src");
	if (sources.empty()) {
		return true;
	}

	// Insert the 'std' module
	if (!ctx.selected_cl.std_module.empty()) {
		sources[0].emplace_back(source_info{ ctx.selected_cl.std_module, {} });
	}

	// Create file containing the list of objects to link
	std::ofstream objects(ctx.output_dir() / "OBJLIST");
	std::shared_mutex mut;

	// Create the build command
	std::string const cmd_prefix = ctx.build_command_prefix();// +resp_args.data();

	// Set up the compiler helper
	bool failed = false;
	auto compile_cpp = [&](source_info const& in) -> void {
		if (failed)
			return;

		auto const& [path, imports] = in;

		fs::path const obj = (ctx.output_dir() / path.filename()).replace_extension("obj");

		{
			std::scoped_lock sl(mut);
			objects << obj << ' ';
		}

		if (is_file_up_to_date(path, obj)) {
			return;
		}

		std::string cmd = cmd_prefix;

		cmd += ctx.build_file(path.string(), obj.string());
		cmd += resp_args.data();

		if (!ctx.selected_cl.reference.empty())
			for (auto const& s : imports)
				cmd += ctx.build_reference(s);

		// Clang/gcc doesn't print out the name of the
		// file being compiled, so do it manually.
		//if (ctx.selected_cl.name != "msvc")
		//	std::puts(path.filename().string().c_str());

		failed = (0 != std::system(cmd.c_str()));
		};

	// Compile sources
#ifdef __clang__
	//#pragma omp parallel for
	for (auto const& paths : std::views::values(sources)) {
		std::for_each(paths.begin(), paths.end(), compile_cpp);
	}
#else
	for (auto const& paths : std::views::values(sources)) {
		std::for_each(std::execution::par_unseq, paths.begin(), paths.end(), compile_cpp);
	}
#endif
	// Close the objects file
	objects.close();

	// Link sources
	if (!failed) {
		std::println("<gbs> Linking...");
		std::string const executable = fs::current_path().stem().string();
		std::string const link = ctx.link_command(executable);
		return 0 == std::system(link.c_str());
	}
	else {
		return false;
	}
}
