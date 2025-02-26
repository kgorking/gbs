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

using namespace std::string_view_literals;
namespace fs = std::filesystem;

static const fs::path msvc_folder = ".gbs/msvc/";
static constexpr std::string_view stdlib_module = "%VCToolsInstallDir%/modules/std.ixx";


// Default response files
static std::unordered_map<std::string_view, std::string_view> response_map = {
	{"_shared", "/nologo /EHsc /std:c++23preview /MP /fastfail "},
	{"debug", "/D_DEBUG /O0 /ifcOutput out/debug/ /Fo:out/debug/"},
	{"release", "/DNDEBUG /O2 /ifcOutput out/release/ /Fo:out/release/"},
	{"analyze", "/analyze:external-"},
};


bool is_file_up_to_date(fs::path const& in, fs::path const& out) {
	return fs::exists(out) && (fs::last_write_time(out) > fs::last_write_time(in));
}


bool ensure_response_file_exists(std::string_view resp) {
	if (resp.empty()) {
		std::println("Error: bad build-arguments. Trailing comma?");
		return false;
	}

	// Check that it is a valid response file
	if (!fs::exists(msvc_folder / resp)) {
		if (!response_map.contains(resp)) {
			std::println("Error: unknown response file {}", resp);
			return false;
		}
		else {
			std::ofstream file(msvc_folder / resp);
			file << response_map[resp];
		}
	}

	return true;
}


bool check_response_files(std::string_view args) {
	if (!fs::exists(msvc_folder)) {
		std::filesystem::create_directories(msvc_folder);
	}

	ensure_response_file_exists("_shared"sv);
	for (auto subrange : args | std::views::split(","sv)) {
		if (!ensure_response_file_exists(std::string_view{ subrange }))
			return false;
	}

	return true;
}

struct compiler {
	int major, minor;
	char name[16];
	char path[232];
};

bool enum_cl(std::string_view args) {
	std::println("Enumerating compilers...");

	extern void enum_cl(std::function<void(compiler)>);
	enum_cl([](compiler c) {
		std::println("Compiler: {:16} {:}.{:<5} {}", c.name, c.major, c.minor, c.path);
		});

	return true;
}


bool build(std::string_view args) {
	std::println("Building...");

	// Ensure the needed response files are present
	if (args.starts_with("build="))
		args.remove_prefix(6);
	check_response_files(args);

	// TODO store path in env var %vcvars%
	int const inst = std::system("\">instpath.txt \"%ProgramFiles(x86)%/Microsoft Visual Studio/Installer/vswhere.exe\" -property installationPath 2>nul\"");
	if (inst != 0) {
		std::println("Error: msvc not found");
		return 1;
	}

	std::string path;
	std::getline(std::ifstream("instpath.txt"), path);
	std::filesystem::remove("instpath.txt");

	// + VC\Tools\MSVC

	std::filesystem::path vcvars(path, std::filesystem::path::generic_format);
	vcvars /= "VC/Auxiliary/Build/vcvars64.bat";

	std::filesystem::create_directories("out/release/bin");

	// Set up the build environment
	std::string cmd = std::format("\"call \"{}\" >nul", vcvars.generic_string());

	// Arguments to the compiler(s)
	auto view_args = args | std::views::split(","sv);				// split args by comma
	auto view_resp = view_args | std::views::join_with(std::format(" @{}", msvc_folder.generic_string()));		// join args with @

	// Compile stdlib if needed
	if (!is_file_up_to_date(stdlib_module, "out/release/std.obj")) {
		cmd += std::format(" && call cl @.gbs/msvc/_shared @.gbs/msvc/{:s} /c \"{}\"", view_resp, stdlib_module);
	}

	// Add source files
	cmd += std::format(" && cl @.gbs/msvc/_shared @.gbs/msvc/{:s} /reference std=out/release/std.ifc /Fe:out/release/bin/test.exe", view_resp);
	auto not_dir = [](fs::directory_entry const& dir) { return !dir.is_directory(); };

	for (auto const& dir : fs::directory_iterator("src") | std::views::filter(not_dir)) {
		fs::path const& path = dir.path();

		auto const out = "out/release" / path.filename().replace_extension("obj");
		cmd += ' ';
		if (is_file_up_to_date(path, out))
			cmd += out.generic_string();
		else
			cmd += path.generic_string();
	}
	cmd += "\"";

	return 0 == std::system(cmd.c_str());
}

bool clean(std::string_view args) {
	std::println("Cleaning...");
	std::filesystem::remove_all("out/release/");
	return true;
}

bool run(std::string_view args) {
	std::println("Running...");
	return 0 == std::system("cd out/release/bin && test.exe");
}

int main(int argc, char const* argv[]) {
	std::println("Gorking build system v0.01\n");

	static std::unordered_map<std::string_view, std::function<bool(std::string_view)>> const commands = {
		{"build", build},
		{"clean", clean},
		{"run", run},
		{"enum_cl", enum_cl},
	};

	auto const args = std::span<char const*>(argv, argc);
	for (std::string_view arg : args | std::views::drop(1)) {
		auto left = arg.substr(0, arg.find('='));
		if (!commands.contains(left)) {
			std::println("Unknown command: {}\n", left);
			return 1;
		}

		if (!commands.at(left)(arg))
			return 1;
	}

	return 0;
}
