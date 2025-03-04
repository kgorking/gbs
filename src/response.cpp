#include <string_view>
#include <filesystem>
#include <fstream>
#include <ranges>
#include <print>
#include "context.h"

namespace fs = std::filesystem;

bool ensure_response_file_exists(context const& ctx, std::string_view resp) {
	if (resp.empty()) {
		std::println("<gbs> Error: bad build-arguments. Trailing comma?");
		return false;
	}

	// Check that it is a valid response file
	if (!fs::exists(ctx.response_dir() / resp)) {
		if (!ctx.response_map.contains(resp)) {
			std::println("<gbs> Error: unknown response file {}", resp);
			return false;
		}
		else {
			std::ofstream file(ctx.response_dir() / resp);
			file << ctx.response_map.at(resp);
		}
	}

	return true;
}


bool check_response_files(context const& ctx, std::string_view args) {
	if (ctx.selected_cl.name.empty()) {
		std::println("Error: select a compiler");
		exit(1);
	}

	if (!fs::exists(ctx.response_dir())) {
		std::filesystem::create_directories(ctx.response_dir());
	}

	ensure_response_file_exists(ctx, "_shared");
	for (auto subrange : args | std::views::split(',')) {
		if (!ensure_response_file_exists(ctx, std::string_view{ subrange }))
			return false;
	}

	return true;
}
