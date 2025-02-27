#include <filesystem>
#include <string_view>
#include <format>
#include <fstream>
#include <functional>
#include "compiler.h"

char const* archs[] = { /*"arm64",*/ "x64" };

void enum_cl(std::function<void(compiler)> callback) {
	std::string line, cmd, version;
	compiler comp;

	int const inst = std::system("\">instpath.txt \"%ProgramFiles(x86)%/Microsoft Visual Studio/Installer/vswhere.exe\" -prerelease -property installationPath 2>nul\"");
	if (inst == 0) {
		std::ifstream file("instpath.txt");

		while (std::getline(file, line)) {
			std::filesystem::path const path(line);

			// Find msvc compilers
			auto const msvc_path = path / "VC\\Tools\\MSVC";
			if (std::filesystem::exists(msvc_path)) {
				comp.name = "msvc";

				for (auto const& dir : std::filesystem::directory_iterator(msvc_path)) {
					for (auto arch : archs) {
						comp.arch = arch;
						comp.path = std::format("{}\\bin\\HostX64\\{}", dir.path().string(), arch);

						if (!std::filesystem::exists(comp.path))
							continue;

						comp.path += "\\cl.exe";
						cmd = std::format(R"("{}" 1>nul 2>version)", comp.path);
						if (0 == std::system(cmd.c_str())) {
							std::getline(std::ifstream("version"), version);

							std::string_view sv(version);
							sv.remove_prefix(sv.find_first_of("0123456789", 0));
							sv = sv.substr(0, sv.find_first_of(' '));

							comp.major = std::atoi(sv.substr(0, sv.find('.')).data());
							comp.minor = std::atoi(sv.substr(sv.find('.') + 1).data());
							callback(comp);
						}
					}
				}
			}

			// Find clang-cl compilers
			auto const llvm_path = path / "VC\\Tools\\LLVM";
			if (std::filesystem::exists(llvm_path)) {
				comp.name = "clang-cl";
				for (auto arch : archs) {
					comp.arch = arch;
					comp.path = std::format("{}\\{}\\bin", llvm_path.string(), arch);

					if (!std::filesystem::exists(comp.path))
						continue;

					comp.path += "\\clang-cl.exe";
					cmd = std::format(R"("{}" -v 2>version)", comp.path);
					if (0 == std::system(cmd.c_str())) {
						std::getline(std::ifstream("version"), version);

						std::string_view sv(version);
						sv.remove_prefix(sv.find_first_of("0123456789", 0));

						comp.major = std::atoi(sv.substr(0, sv.find('.')).data());
						comp.minor = std::atoi(sv.substr(sv.find('.') + 1).data());
						callback(comp);
					}
				}
			}
		}
		file.close();
		std::error_code ec;
		std::filesystem::remove("instpath.txt", ec);
	}

	// Find clang compilers
	// TODO test

	std::error_code ec;
	std::filesystem::remove("version", ec);
}