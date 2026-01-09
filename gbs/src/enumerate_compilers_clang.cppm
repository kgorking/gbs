module;
#include <string>
#include <string_view>
#include <optional>
#include <fstream>
#include <filesystem>
#include <format>
#include <system_error>
#include <cstdio>
export module enumerate_compilers_clang;
import env;
import compiler;

compiler new_compiler(std::string_view version, std::size_t prefix_size) {
	compiler comp;

	comp.name_and_version = version;
	version.remove_prefix(prefix_size); // remove prefix
	extract_compiler_version(version, comp.major, comp.minor, comp.patch);

	comp.name = "clang";
	comp.build_source = " {0:?} -o {1:?} ";
	comp.build_module = " --language=c++-module {0:?} -o {1:?} -fmodule-output ";
	comp.build_command_prefix = "call {0} @{1}/SRC_INCLUDES -c ";
	comp.link_command = "call {0} -o {1}/{2}.exe @{1}/OBJLIST @{1}/LIBLIST";
	comp.slib_command = "call {0} rcs {1}/{2}.lib @{1}/OBJLIST";
	comp.dlib_command = "call {0} -shared -fPIC -o {1}/{2}.dll @{1}/OBJLIST";
	comp.define = "-D";
	comp.include = "-I\"{0}/\"";
	comp.reference = " -fmodule-file={}={}.pcm ";
	comp.target = " -target {} ";

	return comp;
}

std::optional<std::string> find_std_module_path(compiler const& /*comp*/) {
	return std::nullopt;
}

export void enumerate_compilers_clang(environment const& env, auto&& callback) {
	// Enumerate installed clang compilers
	if (0 == std::system("clang --version > clang_version.txt")) {
		std::ifstream version("clang_version.txt");
		std::string line;
		std::getline(version, line);
		if (!line.empty()) {
			compiler comp = new_compiler(line, std::string_view{ line }.find_first_of("0123456789"));
			comp.executable = "clang";
			comp.linker = "clang";
			comp.slib = "llvm-ar";
			comp.dlib = "clang";
			comp.std_module = R"(../../../include/c++/15/bits/std.cc)";

			// Fix the name
			comp.name_and_version = std::format("{}_{}.{}.{}", comp.name, comp.major, comp.minor, comp.patch);

			// Get installed dir
			std::getline(version, line);
			std::getline(version, line);
			std::getline(version, line);
			comp.dir = std::filesystem::path{ line.substr(std::string_view{"InstalledDir: "}.size()) }.generic_string();

			callback(std::move(comp));
		}
	}
	std::remove("clang_version.txt");

	// Enumerate WSL installed clang compilers
	if (0 == std::system("wsl clang --version > clang_version.txt")) {

		// dpkg --list | grep -oE '(g\+\+|clang)\-[[:digit:]]+ '

		std::ifstream version("clang_version.txt");
		std::string line;
		std::getline(version, line);
		if (!line.empty()) {
			compiler comp = new_compiler(line, std::string_view{ line }.find_first_of("0123456789"));

			comp.name = "wsl.clang";
			comp.is_wsl = true;

			std::string const str_major = std::to_string(comp.major);
			comp.executable = "wsl clang++-" + str_major;
			comp.linker = "wsl clang++-" + str_major;
			comp.slib = "wsl llvm-ar-" + str_major;
			comp.dlib = "wsl clang++-" + str_major;
			comp.std_module = "/usr/include/c++/15/bits/std.cc";
			//comp.std_module = R"(\\wsl.localhost\Ubuntu\usr\include\c++\15\bits\std.cc)";
			//comp.std_module = R"(../../../include/c++/15/bits/std.cc)";

			// Fix the name
			comp.dlib_command = "call {0} -shared -fPIC -o {1}/lib{2}.so @{1}/OBJLIST";
			comp.name_and_version = std::format("{}_{}.{}.{}", comp.name, comp.major, comp.minor, comp.patch);

			// Get installed dir
			std::getline(version, line);
			std::getline(version, line);
			std::getline(version, line);
			comp.dir = std::filesystem::path{ line.substr(std::string_view{"InstalledDir: "}.size()) }.generic_string();

			callback(std::move(comp));
		}
	}
	std::remove("clang_version.txt");

	// Find compilers in ~/.gbs/clang
	std::filesystem::path const download_dir = env.get_home_dir() / ".gbs" / "clang";

	if (!std::filesystem::exists(download_dir))
		return;
	
	for (auto const& dir : std::filesystem::directory_iterator(download_dir)) {
		auto const path = dir.path().filename().generic_string();
		std::string_view clang_version = path;

		if (path.starts_with("clang_")) {
			compiler comp = new_compiler(clang_version, 6);

			comp.dir = dir;
			comp.executable = dir.path() / "bin" / "clang";
			comp.linker = comp.executable;
			comp.slib = dir.path() / "bin" / "llvm-ar";
			comp.dlib = comp.executable;

			callback(std::move(comp));
		}
	}
}
