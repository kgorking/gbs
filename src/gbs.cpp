#include <filesystem>
#include <array>
#include <print>
#include <span>
#include <cstdlib>
#include <fstream>
#include <string_view>
#include <ranges>
#include <functional>
#include <regex>
#include "compiler.h"

using namespace std::string_view_literals;
namespace fs = std::filesystem;

static const fs::path gbs_folder = ".gbs";

// Available compilers
compiler_collection all_compilers;
compiler selected_cl;

// Config of last compile (debug, release, etc...)
std::string_view output_config;

// Default response files
static std::unordered_map<std::string_view, std::string_view> response_map = {
	{"_shared", "/nologo /EHsc /std:c++23preview /fastfail /W4 /WX"},// /MP"},
	{"debug", "/Od /MDd /ifcOutput out/debug/ /Fo:out/debug/"},
	{"release", "/DNDEBUG /O2 /MD /ifcOutput out/release/ /Fo:out/release/"},
	{"analyze", "/analyze:external-"},
};


bool ensure_response_file_exists(std::string_view resp) {
	if (resp.empty()) {
		std::println("Error: bad build-arguments. Trailing comma?");
		return false;
	}

	// Check that it is a valid response file
	if (!fs::exists(gbs_folder / selected_cl.name / resp)) {
		if (!response_map.contains(resp)) {
			std::println("Error: unknown response file {}", resp);
			return false;
		}
		else {
			std::ofstream file(gbs_folder / selected_cl.name / resp);
			file << response_map[resp];
		}
	}

	return true;
}


bool check_response_files(std::string_view args) {
	if (selected_cl.name.empty()) {
		std::println("Error: select a compiler");
		exit(1);
	}

	if (!fs::exists(gbs_folder / selected_cl.name)) {
		std::filesystem::create_directories(gbs_folder / selected_cl.name);
	}

	ensure_response_file_exists("_shared"sv);
	for (auto subrange : args | std::views::split(","sv)) {
		if (!ensure_response_file_exists(std::string_view{ subrange }))
			return false;
	}

	return true;
}

bool enum_cl(std::string_view args) {
	std::println("Enumerating compilers:");

	fill_compiler_collection();

	for(auto& [k, v] : all_compilers) {
		std::println("{}: ", k);
		for (auto const &c : v) {
			std::println("  {}.{} - {}", c.major, c.minor, c.dir.generic_string());
		}
	}

	return true;
}


bool build(std::string_view args) {
	// Default build config is 'debug'
	if (args.empty())
		args = "debug";

	// Ensure the needed response files are present
	check_response_files(args);

	// Set up the build environment
	auto const vcvars = selected_cl.dir / "../../../Auxiliary/Build/vcvars64.bat";
	std::string cmd = std::format("\"\"{}\" >nul", vcvars.generic_string());

	// Arguments to the compiler(s)
	auto view_args = args | std::views::split(","sv);				// split args by comma
	auto view_resp = view_args | std::views::join_with(std::format(" @{}", (gbs_folder / selected_cl.name).generic_string()));		// join args with @

	// Build output
	std::string const executable = fs::current_path().stem().string() + ".exe";
	output_config = std::string_view{ view_args.front() };
	auto output_dir = fs::path("out") / output_config;

	// Create the build dirs if needed
	std::filesystem::create_directories(output_dir / "bin");

	// Determine response file folder
	std::string const response_folder = std::format("@.gbs/{}", selected_cl.name);

	// Compile stdlib if needed
	if (!fs::exists(output_dir / "std.obj")) {
		fs::path const stdlib_module = selected_cl.dir / "modules/std.ixx";
		if (!is_file_up_to_date(stdlib_module, output_dir / "std.obj"))
			cmd += std::format(" && call cl {0}/_shared {0}/{1:s} /c \"{2}\"", response_folder, view_resp, stdlib_module.generic_string());
	}

	// Add source files
	cmd += std::format(" && cl {0}/_shared {0}/{1:s} /reference std={2}/std.ifc /Fe:{2}/bin/{3}", response_folder, view_resp, output_dir.generic_string(), executable);
	auto not_dir = [](fs::directory_entry const& dir) { return !dir.is_directory(); };

	extern void enumerate_sources(std::filesystem::path, std::filesystem::path);
	enumerate_sources("src", output_dir);

	cmd += " @out/modules @out/sources @out/objects";

	cmd += "\"";
	std::puts(cmd.c_str());
	return 0 == std::system(cmd.c_str());
}

bool clean(std::string_view args) {
	std::println("Cleaning...");
	std::filesystem::remove_all("out");
	return true;
}

bool run(std::string_view args) {
	if (!args.empty())
		output_config = args.substr(0, args.find_first_of(',', 0));

	if (output_config.empty()) {
		std::println("Error: run : don't know what to run! Call 'run' after a compilation, or use eg. 'run=release' to run the release build.");
		exit(1);
	}

	std::string const executable = fs::current_path().stem().string() + ".exe";
	std::println("Running '{}' ({})...\n", executable, output_config);
	return 0 == std::system(std::format("cd out/{}/bin && {}", output_config, executable).c_str());
}


bool cl(std::string_view args) {
	if (all_compilers.empty()) {
		fill_compiler_collection();
		if (all_compilers.empty()) {
			std::println("Error: no compilers found.");
			exit(1);
		}
	}

	selected_cl = get_compiler(args);
	std::println("Using compiler '{} v{}.{}'", selected_cl.name, selected_cl.major, selected_cl.minor);
	return true;
}


int main(int argc, char const* argv[]) {
	std::println("Gorking build system v0.02\n");

	static std::unordered_map<std::string_view, std::function<bool(std::string_view)>> const commands = {
		{"build", build},
		{"clean", clean},
		{"run", run},
		{"enum_cl", enum_cl},
		{"cl", cl},
	};

	auto const args = std::span<char const*>(argv, argc);
	for (std::string_view arg : args | std::views::drop(1)) {
		auto left = arg.substr(0, arg.find('='));
		if (!commands.contains(left)) {
			std::println("Unknown command: {}\n", left);
			return 1;
		}

		arg.remove_prefix(left.size());
		arg.remove_prefix(!arg.empty() && arg.front() == '=');
		if (!commands.at(left)(arg))
			return 1;
	}

	return 0;
}
