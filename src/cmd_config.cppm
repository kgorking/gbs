export module cmd_config;
import std;
import context;
import monad;

// Converts arguments into response files
std::string convert_arg_to_response(std::string_view arg, std::filesystem::path const& response_dir) {
	return std::format(" @{}/{}", response_dir.string(), arg);
}

export bool cmd_config(context& ctx, std::string_view args) {
	// Set the default build config if none is specified
	if (args.empty())
		args = "release,warnings";

	// Ensure the needed response files are present
	ctx.check_response_files(args);

	// Set the build configuration
	// and create the build dirs if needed
	ctx.set_config(args);

	// Arguments to the compiler.
	ctx.set_response_args(as_monad(args)
		.join()
		.split(',')
		.prefix("_shared")
		.map(convert_arg_to_response, ctx.response_dir())
		.to<std::string>()
		);

	return true;
}
