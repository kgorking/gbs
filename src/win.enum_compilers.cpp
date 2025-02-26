#include <filesystem>
#include <string_view>
#include <format>
#include <fstream>
#include <functional>

struct compiler {
	int major, minor;
	char name[16];
	char path[232];
};

void enum_cl(std::function<void(compiler)> callback) {
	int const inst = std::system("\">instpath.txt \"%ProgramFiles(x86)%/Microsoft Visual Studio/Installer/vswhere.exe\" -prerelease -property installationPath 2>nul\"");
	if (inst != 0) {
		return;
	}

	std::string line, cmd, version;
	compiler comp;
	std::ifstream file("instpath.txt");

	while (std::getline(file, line)) {
		std::filesystem::path const path(line);

		// Find msvc compilers
		auto const msvc_path = path / R"(VC\Tools\MSVC)";
		if (std::filesystem::exists(msvc_path)) {
			std::format_to_n(comp.name, sizeof(comp.name), "msvc{}", '\0');

			for (auto const& dir : std::filesystem::directory_iterator(path / R"(VC\Tools\MSVC)")) {
				std::format_to_n(comp.path, sizeof(comp.path), "{}\\bin\\HostX64\\x64\\cl.exe{}", dir.path().string(), '\0');

				cmd = std::format(R"("{}" 1>nul 2>version)", comp.path);
				std::system(cmd.c_str());

				std::getline(std::ifstream("version"), version);

				std::string_view sv(version);
				sv.remove_prefix(sv.find_first_of("0123456789", 0));
				sv = sv.substr(0, sv.find_first_of(' '));

				comp.major = std::stoi(std::string(sv.substr(0, sv.find('.'))));
				comp.minor = std::stoi(std::string(sv.substr(sv.find('.') + 1)));
				callback(comp);
			}
		}

		// Find clang compilers
		auto const llvm_path = path / R"(VC\Tools\LLVM)";
		if (std::filesystem::exists(llvm_path)) {
			std::format_to_n(comp.name, sizeof(comp.name), "clang-cl{}", '\0');
			std::format_to_n(comp.path, sizeof(comp.path), R"({}\bin\clang-cl.exe{})", llvm_path.string(), '\0');

			cmd = std::format(R"("{}" -v 2>version)", comp.path);
			std::system(cmd.c_str());

			std::getline(std::ifstream("version"), version);

			std::string_view sv(version);
			sv.remove_prefix(sv.find_first_of("0123456789", 0));

			comp.major = std::stoi(std::string(sv.substr(0, sv.find('.'))));
			comp.minor = std::stoi(std::string(sv.substr(sv.find('.') + 1)));
			callback(comp);
		}
	}

	file.close();
	std::error_code ec;
	std::filesystem::remove("version", ec);
	std::filesystem::remove("instpath.txt", ec);
}