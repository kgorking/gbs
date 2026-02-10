module;
#include <filesystem>
#include <iostream>
#include <print>
export module cmd_unittest;
import context;
import wsl;

namespace fs = std::filesystem;

export bool cmd_unittest(context& ctx, std::string_view args) {
	if (ctx.get_config().empty()) {
		std::println(std::cerr, "<gbs> No build configuration selected. Please set 'config=<...>' before running unittests.");
		return false;
	}

	if (!fs::exists(ctx.output_dir())) {
		std::println(std::cerr, "<gbs> Build directory is missing. Build the project before running tests.");
		return false;
	}

	int num_tests_run = 0;
	auto test = fs::current_path();

	for (auto const& unittest : ctx.get_unittests()) {
		std::println("<gbs> Running unittest \"{0}\"", unittest.filename().generic_string());

		std::string const wsl = get_wsl_command(ctx.get_selected_compiler().wsl);
		auto const cmd = std::format("{}\"{}\" {}", wsl, unittest.generic_string(), args);
		std::system(cmd.c_str());
		//if (0 != std::system(cmd.c_str())) {
		//	std::println(std::cerr, "<gbs> Unittest '{}' failed.", unittest.filename().generic_string());
		//	return false;
		//}

		num_tests_run += 1;
	}

	if (num_tests_run == 0) {
		std::println("<gbs> No unittests found to run.");
	}
	return true;
}