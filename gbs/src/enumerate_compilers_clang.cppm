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
import wsl;

compiler new_compiler(std::string_view version, std::size_t prefix_size) {
	compiler comp;

	comp.name_and_version = version;
	version.remove_prefix(prefix_size); // remove prefix
	extract_compiler_version(version, comp.major, comp.minor, comp.patch);
	comp.name_and_version = std::format("{}_{}.{}.{}", comp.name, comp.major, comp.minor, comp.patch);

	comp.name = "clang";
	comp.build_source = " {0:?} -o {1:?} ";
	comp.build_module = " --language=c++-module {0:?} -o {1:?} -fmodule-output ";
	comp.build_command_prefix = "call {0} @{1}/SRC_INCLUDES -c ";
	comp.link_command = "call {0} -o {1}/{2} @{1}/OBJLIST @{1}/LIBLIST";
	comp.slib_command = "call {0} rcs {1}/{2}.lib @{1}/OBJLIST";
	comp.dlib_command = "call {0} -shared -fPIC -o {1}/{2} @{1}/OBJLIST";
	comp.define = "-D";
	comp.include = "-I\"{0}/\"";
	comp.reference = " -fmodule-file={}={}.pcm ";
	comp.target = " -target {} ";


	return comp;
}

std::optional<std::string> find_std_module_path(compiler const& comp, bool is_windows) {
	// Select the correct NULL device based on the operating system
#ifdef WIN32
	std::string_view const null_device = "NUL";
#else
	std::string_view const null_device = "/dev/null";
#endif
	std::string const std_module_file = is_windows ? "\\..\\modules\\std.ixx" : "bits/std.cc";

	// Create a command to get the include paths from the compiler
	std::string const command = std::format("{} -v -E -x c++ - < {} > compiler_output.txt 2>&1", comp.executable.generic_string(), null_device);

	// Execute the command and look for the standard module in the include paths
	if (0 == std::system(command.c_str())) {
		if (std::ifstream output("compiler_output.txt"); output) {
			// Find the start of the include paths
			std::string line;
			while (output && line != "#include <...> search starts here:") {
				std::getline(output, line);
			}

			// Read each include path and check for the standard module file.
			// Include paths start with a space.
			while (std::getline(output, line) && line[0] == ' ') {
				line = std::filesystem::path(line.substr(1) + std_module_file).lexically_normal().generic_string();

				if (comp.wsl) {
					std::string const path = comp.wsl ? std::format(R"(\\wsl.localhost\{}{})", *comp.wsl, line) : line;

					if (std::filesystem::exists(path))
						break;
				} else {
					if (std::filesystem::exists(line))
						break;
				}
			}

			std::remove("compiler_output.txt");
			if (line != "End of search list.")
				return line;
		}
	}

	return std::nullopt;
}

export void enumerate_compilers_clang(environment const& env, auto&& callback) {
	// Enumerate installed clang compiler
	if (0 == std::system("clang --version > clang_version.txt 2>&1")) {
		std::ifstream file("clang_version.txt");
		std::string line;
		std::getline(file, line);
		if (!line.empty()) {
			compiler comp = new_compiler(line, std::string_view{ line }.find_first_of("0123456789"));

			// Get installed dir
			std::getline(file, line);
			bool const is_windows = line.contains("-windows-");
			std::getline(file, line);
			std::getline(file, line);
			comp.dir = std::filesystem::path{ line.substr(std::string_view{"InstalledDir: "}.size()) }.generic_string();

			comp.executable = "clang";
			comp.linker = "clang";
			comp.slib = "llvm-ar";
			comp.dlib = "clang";
			comp.std_module = find_std_module_path(comp, is_windows);

			callback(std::move(comp));
		}
	}
	std::remove("clang_version.txt");

	// Enumerate WSL installed clang compilers
	auto const wsl_distros = get_wsl_distributions();
	for (std::string const& distro : wsl_distros) {
		std::string const wsl_prefix = "wsl -d " + distro + " ";
		std::string const command = wsl_prefix + "clang --version > clang_version.txt 2>&1";

		if (0 == std::system(command.c_str())) {
			std::ifstream version("clang_version.txt");
			std::string line;
			std::getline(version, line);
			if (!line.empty()) {
				compiler comp = new_compiler(line, std::string_view{ line }.find_first_of("0123456789"));

				comp.name = "clang";
				comp.wsl = distro;

				std::string const str_major = std::to_string(comp.major);
				comp.executable = wsl_prefix + "clang++-" + str_major;
				comp.linker = wsl_prefix + "clang++-" + str_major;
				comp.slib = wsl_prefix + "llvm-ar-" + str_major;
				comp.dlib = wsl_prefix + "clang++-" + str_major;
				comp.std_module = find_std_module_path(comp, false);

				comp.dlib_command = "call {0} -shared -fPIC -o {1}/{2} @{1}/OBJLIST";

				// Get installed dir
				std::getline(version, line);
				std::getline(version, line);
				std::getline(version, line);
				comp.dir = std::filesystem::path{ line.substr(std::string_view{"InstalledDir: "}.size()) }.generic_string();

				callback(std::move(comp));
			}
		}
		std::remove("clang_version.txt");
	}

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
#ifdef WIN32
			bool constexpr is_windows = true;
#else
			bool constexpr is_windows = false;
#endif
			comp.std_module = find_std_module_path(comp, is_windows);

			callback(std::move(comp));
		}
	}
}
