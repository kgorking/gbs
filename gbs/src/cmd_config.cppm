export module cmd_config;
import std;
import context;

// Converts arguments into response files
std::string convert_arg_to_response(std::string_view arg, context const& ctx) {
	return std::format(" @{}/{}", ctx.response_dir().generic_string(), arg);
}

export bool cmd_config(context& ctx, std::string_view args) {
	// Set the default build config if none is specified
	if (args.empty())
		args = "release,warnings";

	// Ensure the necessary response files are present
	ctx.check_response_files(args);

	// Set the build configuration
	// and create the build dirs if needed
	ctx.set_config(args);

	// Arguments to the compiler.
	std::string response_args = convert_arg_to_response("_shared", ctx);
	while (!args.empty()) {
		std::size_t const index = args.find(',');
		response_args += convert_arg_to_response(args.substr(0, index), ctx);
		args.remove_prefix(index == std::string_view::npos ? args.size() : index + 1);
	}

	ctx.set_response_args(std::move(response_args));

	return true;
}
