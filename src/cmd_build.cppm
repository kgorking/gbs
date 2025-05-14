export module cmd_build;
import std;
import env;
import context;
import response;
import get_source_groups;

namespace fs = std::filesystem;
using namespace std::string_view_literals;
using namespace std::views;

export bool cmd_build(context& ctx, std::string_view args) {
	// Select default compiler if none is selected
	if (ctx.selected_cl.name.empty()) {
		if (ctx.all_compilers.empty()) {
			fill_compiler_collection(ctx);
		}
		ctx.select_first_compiler();
	}

	std::println("<gbs> Building with '{} {}.{}'", ctx.selected_cl.name, ctx.selected_cl.major, ctx.selected_cl.minor);

	if (!fs::exists("src/")) {
		std::println("<gbs> Error: no 'src' directory found at '{}'", fs::current_path().generic_string());
		return false;
	}

	// Default build config is 'release'
	if (args.empty())
		args = "release";

	// Ensure the needed response files are present
	init_response_files(ctx);
	check_response_files(ctx, args);

	// Build output
	ctx.config = args.substr(0, args.find(','));
	auto const output_dir = ctx.output_dir();

	// Create the build dirs if needed
	fs::create_directories(ctx.output_dir());

	// Arguments to the compiler.
	// Converts arguments into response files
	auto arg_to_str = [&](auto arg) { return " @" + (ctx.response_dir() / arg.data()).string(); };

	// TODO std::views::concat("_shared", args)
	std::string resp_args = arg_to_str("_shared"sv);
	resp_args += args
		| split(',')
		| transform(arg_to_str)
		| join
		| std::ranges::to<std::string>();

	if (ctx.selected_cl.name == "msvc") {
		extern bool init_msvc(context&);
		if (!init_msvc(ctx))
			return false;

		//extern bool build_msvc(context &, std::string_view);
		//return build_msvc(ctx, resp_args);
	}
	else if (ctx.selected_cl.name.starts_with("clang")) {
		//extern bool build_clang(context &, std::string_view);
		//return build_clang(ctx, resp_args);
	}
	else if (ctx.selected_cl.name == "gcc") {
		std::println("<gbs> INTERNAL : not implemented yet");
		return false;
		//	extern bool build_gcc(context & ctx, std::string_view args);
		//	return build_gcc(ctx, args);
	}
	else {
		std::println("<gbs> INTERNAL : unknown compiler {:?}", ctx.selected_cl.name);
		return false;
	}

	// Get the source files to compile
	auto sources = get_grouped_source_files("src");
	if (sources.empty())
		return true;

	// Insert the 'std' module
	if (!ctx.selected_cl.std_module.empty())
		sources[0].emplace_back(source_info{ ctx.selected_cl.std_module, {} });

	// Create file containing the list of objects to link
	std::ofstream objects(ctx.output_dir() / "OBJLIST");
	std::shared_mutex mut;

	// Create the build command
	std::string const cmd_prefix = ctx.build_command_prefix() + resp_args.data();

	// Set up the compiler helper
	auto compile_cpp = [&](this auto& self, source_info const& in) -> bool {
		auto const& [path, imports] = in;

		fs::path const obj = (ctx.output_dir() / path.filename()).replace_extension("obj");

		{
			std::scoped_lock sl(mut);
			objects << obj << ' ';
		}

		if (is_file_up_to_date(path, obj)) {
			return true;
		}

		std::string cmd = cmd_prefix + ctx.build_file(path.string(), obj.string());
		for (auto const& s : imports)
			cmd += ctx.build_reference(s);

		//std::puts(cmd.c_str());
		if (ctx.selected_cl.name != "msvc")
			std::puts(path.filename().string().c_str());

		return (0 == std::system(cmd.c_str()));
		};

	// Compile sources
	for (auto const& paths : sources) {
		std::for_each(std::execution::par_unseq, paths.begin(), paths.end(), compile_cpp);
	}

	// Close the objects file
	objects.close();

	// Link sources
	std::println("<gbs> Linking...");
	std::string const executable = fs::current_path().stem().string();
	std::string const link = ctx.link_command(executable);
	return 0 == std::system(link.c_str());
}
