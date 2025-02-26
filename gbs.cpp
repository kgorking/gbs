#include <filesystem>
#include <array>
#include <print>
#include <span>
#include <cstdlib>
#include <fstream>
#include <string_view>
#include <ranges>
#include <functional>

using namespace std::string_view_literals;
namespace fs = std::filesystem;

static constexpr auto cl_shared  = "/nologo /EHsc /std:c++23preview /MP /fastfail "sv;
static constexpr auto cl_debug   = "/D_DEBUG /O0 /ifcOutput out/debug/   /Fo:out/debug/"sv;
static constexpr auto cl_release = "/DNDEBUG /O2 /ifcOutput out/release/ /Fo:out/release/"sv;


bool is_file_up_to_date(fs::path const& in, fs::path const& out) {
	return fs::exists(out) && (fs::last_write_time(out) > fs::last_write_time(in));
}


bool build(std::string_view args) {
	std::println("Building...");
	int const inst = std::system("\">instpath.txt \"%ProgramFiles(x86)%/Microsoft Visual Studio/Installer/vswhere.exe\" -property installationPath 2>nul\"");
	if (inst != 0) {
		std::println("Error: msvc not found");
		return 1;
	}

	std::string path;
	std::getline(std::ifstream("instpath.txt"), path);
	std::filesystem::remove("instpath.txt");

	std::filesystem::path vcvars(path, std::filesystem::path::generic_format);
	vcvars /= "VC/Auxiliary/Build/vcvars64.bat";

	std::filesystem::create_directories("out/release/bin");

	// Arguments to the compiler(s)
	std::string cl_args;
	cl_args += cl_shared;
	cl_args += cl_release;

	std::string cmd = std::format("\"call \"{}\" >nul", vcvars.generic_string());

	// Compile stdlib if needed
	#define STDLIB_MODULE R"(%VCToolsInstallDir%/modules/std.ixx)"
	if (!is_file_up_to_date(STDLIB_MODULE, "out/release/std.obj")) {
		cmd += " && call copy /y \"%VCToolsInstallDir%modules\\std.ixx\" \"out/release/std.ixx\" 1>nul";
		cmd += std::format(" && cl {} /c out/release/std.ixx", cl_args);
	}

	// Add source files
	cmd += std::format(" && cl {} /reference std=out/release/std.ifc /Fe:out/release/bin/test.exe", cl_args);
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

	auto const args = std::span<char const*>(argv, argc);
	for (std::string_view arg : args | std::views::drop(1)) {
		if (arg.starts_with("build")) {
			if (!build(arg))
				return 1;
		}
		else if (arg.starts_with("clean")) {
			if (!clean(arg))
				return 1;
		}
		else if (arg.starts_with("run")) {
			if (!run(arg))
				return 1;
		}
		else {
			std::println("Unknown command: {}\n", arg);
			return 1;
		}
	}

	return 0;
}
