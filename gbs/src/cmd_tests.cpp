#include "../inc/compiler.h"
#include "../inc/context.h"
#include "../inc/wsl.h"
#include <filesystem>
#include <iostream>
#include <print>

namespace fs = std::filesystem;

bool cmd_unittest(context& ctx, std::string_view args) {
	if (ctx.get_config().empty()) {
		std::println(std::cerr, "<gbs> No build configuration selected. Please set 'config=<...>' before running unittests.");
		return false;
	}

	auto const& unittests = ctx.get_unittests();
	if (unittests.empty()) {
		std::println("<gbs> No tests found to run.");
		return true;
	}

	std::println("<gbs> Running tests...");
	for (auto const& unittest : unittests) {
		if (!fs::exists(unittest.executable)) {
			std::println(std::cerr, "<gbs> Test executable '{}' is missing. Build the project before running tests.", unittest.executable.generic_string());
			return false;
		}

		std::string const wsl = get_wsl_command(unittest.wsl);
		auto const cmd = std::format("{}\"{}\" {}", wsl, unittest.executable.generic_string(), args);
		if (0 != std::system(cmd.c_str())) {
			return false;
		}
	}

	return true;
}
