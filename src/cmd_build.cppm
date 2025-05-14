export module cmd_build;
import std;
import context;
import response;

namespace fs = std::filesystem;

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
	ctx.output_config = args.substr(0, args.find(','));
	auto const output_dir = ctx.output_dir();

	// Create the build dirs if needed
	fs::create_directories(ctx.output_dir());

	// Arguments to the compiler.
	// Converts arguments into response files
	auto const resp_prefix = ctx.response_dir();
	auto arg_to_str = [&](auto arg) { return " @" + (resp_prefix / arg.data()).string(); };

	std::string resp_args = "@" + (resp_prefix / "_shared").string();
	resp_args += args
		| std::views::split(',')
		| std::views::transform(arg_to_str)
		| std::views::join
		| std::ranges::to<std::string>();

	if (ctx.selected_cl.name == "msvc") {
		extern bool build_msvc(context &, std::string_view);
		return build_msvc(ctx, resp_args);
	} if (ctx.selected_cl.name.starts_with("clang")) {
		extern bool build_clang(context &, std::string_view);
		return build_clang(ctx, resp_args);
		//} if (ctx.selected_cl.name == "gcc") {
		//	extern bool build_gcc(context & ctx, std::string_view args);
		//	return build_gcc(ctx, args);
	}
	else {
		std::println("<gbs> INTERNAL : not implemented yet");
		return false;
	}
}
