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

	std::filesystem::create_directory("out");

	// Arguments to the compiler(s)
	auto cl_args = "/nologo /EHsc /std:c++23preview /MP /fastfail /O2 /ifcOutput out\\ /Foout\\";

	std::string cmd = std::format("\"\"{}\" >nul", vcvars.generic_string());

	// Compile stdlib if needed
	//#define STDLIB_MODULE R"(%VCToolsInstallDir%/modules/std.ixx)"
	#define STDLIB_MODULE R"(C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\MSVC\14.44.34823\modules/std.ixx)"
	if (!is_file_up_to_date(STDLIB_MODULE, "out\\std.obj")) {
		cmd += std::format(" && cl {} /c \"{}\"", cl_args, STDLIB_MODULE);
	}

	// Add source files
	cmd += std::format(" && cl {} /reference std=out\\std.ifc /Fe:out\\test.exe", cl_args);
	auto not_dir = [](fs::directory_entry const& dir) { return !dir.is_directory(); };

	for (auto const& dir : fs::directory_iterator("src") | std::views::filter(not_dir)) {
		fs::path const& path = dir.path();

		auto const out = "out" / path.filename().replace_extension("obj");
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
	std::filesystem::remove_all("out");
	return true;
}

bool run(std::string_view args) {
	std::println("Running...");
	return 0 == std::system("out\\test.exe");
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
