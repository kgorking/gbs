#include <string_view>
#include <filesystem>
#include <fstream>
#include <ranges>
#include <print>
#include "context.h"

namespace fs = std::filesystem;

void init_response_files(context& ctx) {
	// Default response files
	ctx.response_map["msvc"] = {
		{"_shared", "/nologo /EHsc /std:c++23preview /fastfail /W4 /WX"},
		{"debug", "/Od /MDd /ifcOutput gbs.out/msvc/debug/ /Fo:gbs.out/msvc/debug/"},
		{"release", "/DNDEBUG /O2 /MD /ifcOutput gbs.out/msvc/release/ /Fo:gbs.out/msvc/release/"},
		{"analyze", "/analyze:plugin EspXEngine.dll /analyze:external-"}
	};

	ctx.response_map["clang"] = {
		{"_shared", "-std=c++2b -Wall -Werror"},
		{"debug", "-O0"},
		{"release", "-O3"},
		{"analyze", "--analyze"}
	};

	ctx.response_map["gcc"] = ctx.response_map["clang"];

	//ctx.response_map["clang-cl"] = {
	//	{"_shared", "/std:c++2b /Wall /Werror"},
	//	{"debug", "/O0"},
	//	{"release", "/O3"},
	//	{"analyze", "/analyze"}
	//};
}

bool ensure_response_file_exists(context const& ctx, std::string_view resp) {
	if (resp.empty()) {
		std::println("<gbs> Error: bad build-arguments. Trailing comma?");
		return false;
	}

	auto& map = ctx.response_map.at(ctx.selected_cl.name);

	// Check that it is a valid response file
	if (!fs::exists(ctx.response_dir() / resp)) {
		if (!map.contains(resp)) {
			std::println("<gbs> Error: unknown response file {}", resp);
			return false;
		}
		else {
			std::ofstream file(ctx.response_dir() / resp);
			file << map.at(resp);
		}
	}

	return true;
}


bool check_response_files(context const& ctx, std::string_view args) {
	if (ctx.selected_cl.name.empty()) {
		std::println("Error: select a compiler");
		exit(1);
	}

	if (!ctx.response_map.contains(ctx.selected_cl.name)) {
		std::println("Error: selected compiler does not have any default response files");
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
