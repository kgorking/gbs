#include "../inc/commands.h"
#include "../inc/context.h"
#include <format>
#include <print>
#include <string>

// Converts arguments into response files
std::string convert_arg_to_response(std::string_view arg, context const& ctx, compiler const& cl) {
	return std::format(" @{}/{}", ctx.response_dir(cl).generic_string(), arg);
}

bool cmd_config(context& ctx, compiler const& cl, std::string_view args) {
	// Set the default build config if none is specified
	if (args.empty())
		args = "release,warnings";

	// Ensure the necessary response files are present
	ctx.check_response_files(cl, args);

	// Set the build configuration
	// and create the build dirs if needed
	ctx.set_config(cl, args);

	// Arguments to the compiler.
	std::string response_args = convert_arg_to_response("_shared", ctx, cl);
	while (!args.empty()) {
		std::size_t const index = args.find(',');
		response_args += convert_arg_to_response(args.substr(0, index), ctx, cl);
		args.remove_prefix(index == std::string_view::npos ? args.size() : index + 1);
	}

	ctx.set_response_args(std::move(response_args));

	return true;
}

bool cmd_config(context& ctx, std::string_view args) {
	auto const& compilers = ctx.get_selected_compilers();
	if (compilers.empty()) {
		std::println("<gbs> Error: select a compiler");
		return false;
	}

	for (auto& cl : compilers) {
		if (!cmd_config(ctx, cl, args))
			return false;
	}

	return true;
}
