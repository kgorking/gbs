#include "../inc/compiler.h"
#include "../inc/env.h"
#include "../inc/wsl.h"
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <iostream>
#include <optional>
#include <print>
#include <set>
#include <string>
#include <string_view>
#include <system_error>

namespace {
#ifdef _WIN32
std::string_view gcc_version_from_output(std::string_view line) {
	// Skip the compiler name and target suffix in the first --version line.
	for (std::size_t i = 0; i < line.size(); ++i) {
		bool const previous_is_digit = i > 0 && line[i - 1] >= '0' && line[i - 1] <= '9';
		if (previous_is_digit || line[i] < '0' || line[i] > '9')
			continue;

		std::size_t end = i;
		while (end < line.size() && line[end] >= '0' && line[end] <= '9')
			++end;

		if (end < line.size() && line[end] == '.')
			return line.substr(i);
	}
	return {};
}

std::optional<std::string> find_std_module_path(compiler const& comp) {
#ifdef _WIN32
	std::string_view const null_device = "NUL";
#else
	std::string_view const null_device = "/dev/null";
#endif
	std::string const command = std::format("{} -v -E -x c++ - < {} > compiler_output.txt 2>&1", comp.executable.generic_string(), null_device);

	if (0 == std::system(command.c_str())) {
		if (std::ifstream output("compiler_output.txt"); output) {
			std::string line;
			while (output && line != "#include <...> search starts here:")
				std::getline(output, line);

			while (std::getline(output, line) && !line.empty() && line[0] == ' ') {
				line = std::filesystem::path(line.substr(1) + "/bits/std.cc").lexically_normal().string();

				if (comp.wsl) {
					if (std::filesystem::exists(std::format(R"(\\wsl.localhost\{}{})", *comp.wsl, line)))
						break;
				}
				else if (std::filesystem::exists(line))
					break;
			}

			output.close();
			std::error_code ec;
			std::filesystem::remove("compiler_output.txt", ec);
			if (line != "End of search list.")
				return line;
		}
	}

	return std::nullopt;
}

std::optional<compiler> make_wsl_gcc_compiler(std::string const& distro, std::string const& executable_name) {
	std::string const wsl_prefix = "wsl -d " + distro + " ";
	std::string const command = wsl_prefix + executable_name + " --version > gcc_version.txt 2>&1";
	if (0 != std::system(command.c_str()))
		return std::nullopt;

	std::string first_line;
	if (std::ifstream version("gcc_version.txt"); version)
		std::getline(version, first_line);

	std::error_code ec;
	std::filesystem::remove("gcc_version.txt", ec);

	std::string_view const version_sv = gcc_version_from_output(first_line);
	if (version_sv.empty())
		return std::nullopt;

	compiler comp;
	extract_compiler_version(version_sv, comp.major, comp.minor, comp.patch);
	if (comp.major < 15)
		return std::nullopt;

	comp.name = "gcc/" + distro;
	comp.name_and_version = std::format("gcc_{}_{}.{}.{}", distro, comp.major, comp.minor, comp.patch);
	comp.wsl = distro;
	comp.dir = "/usr/bin";

	comp.executable = wsl_prefix + executable_name;
	comp.linker = comp.executable;
	comp.slib = wsl_prefix + "gcc-ar-" + std::to_string(comp.major);
	comp.dlib = comp.executable;

	comp.build_source = " {0:?} -o {1:?} ";
	comp.build_module = " -xc++ {0:?} -o {1:?} ";
	comp.build_prefix = "call {0} @{1}/SRC_INCLUDES -c -fPIC ";

	comp.link_command = "call {0} -o {1}/{2}";
	comp.slib_command = "call {0} rcs {1}/{2}";
	comp.dlib_command = "call {0} -shared -fPIC -o {1}/{2}";

	comp.define = "-D";
	comp.include = "-I{0}";
	comp.link_libraries = " -lstdc++exp";

	// GCC's default mapper is concurrency-safe and is available on WSL,
	// unlike g++-mapper-server on many distro installations.
	comp.module_path = {};
	comp.std_module = find_std_module_path(comp);

	return comp;
}
#endif
} // namespace

void enumerate_compilers_gcc(environment const& env, std::function<void(compiler&&)> callback) {
#ifdef _WIN32
	// Enumerate versioned WSL g++ compilers (g++-15, g++-16, ...).
	for (std::string const& distro : get_wsl_distributions()) {
		std::string const wsl_prefix = "wsl -d " + distro + " ";
		std::string const command = wsl_prefix
			+ R"(bash -lc "ls /usr/bin/g++-[0-9]* /usr/local/bin/g++-[0-9]* 2>/dev/null || true")"
			+ " > gcc_versions.txt 2>nul";

		if (0 == std::system(command.c_str())) {
			std::set<std::string> seen;
			if (std::ifstream versions("gcc_versions.txt"); versions) {
				std::string line;
				while (std::getline(versions, line)) {
					if (line.empty())
						continue;

					std::string const executable_name = std::filesystem::path{ line }.filename().generic_string();
					std::string_view const executable_version = executable_name;
					if (!executable_version.starts_with("g++-"))
						continue;

					std::string const version = std::string{ executable_version.substr(4) };
					if (std::atoi(version.c_str()) < 15 || !seen.insert(version).second)
						continue;

					if (std::optional<compiler> comp = make_wsl_gcc_compiler(distro, executable_name))
						callback(std::move(*comp));
				}
			}
		}

		std::error_code ec;
		std::filesystem::remove("gcc_versions.txt", ec);
	}
#endif

	// Find compilers in ~/.gbs/*
	auto const download_dir = env.get_home_dir() / ".gbs" / "gcc";

	if (!std::filesystem::exists(download_dir))
		return;

	for (auto const& dir : std::filesystem::directory_iterator(download_dir)) {
		auto const path = dir.path().filename().generic_string();
		std::string_view gcc_version = path;

		if (path.starts_with("gcc_")) {
			compiler comp;

			// gcc_xx.y.z-w
			comp.name_and_version = gcc_version;
			gcc_version.remove_prefix(4);
			extract_compiler_version(gcc_version, comp.major, comp.minor, comp.patch);

			if (comp.major < 15) {
				//std::println(std::cerr, "<gbs> warning: Detected GCC version {}. Only GCC 15 and later is supported.", gcc_version);
				continue;
			}

			auto const& actual_path = dir.path();

			comp.name = "gcc";
			comp.dir = dir;
			comp.executable = actual_path / "bin" / "g++";
			comp.linker = comp.executable;
			comp.slib = actual_path / "bin" / "ar";
			comp.dlib = actual_path / "bin" / "g++";

			if (comp.major >= 15) {
				comp.std_module = actual_path / "include" / "c++" / comp.name_and_version.substr(4) / "bits" / "std.cc";
				if (!std::filesystem::exists(*comp.std_module)) {
					std::println(std::cerr, "<gbs> Error: Could not find 'std.cc' at location {}", comp.std_module->generic_string());
					std::println(std::cerr, "<gbs>        `import std;` will not be available.");
					comp.std_module.reset();
				}
			}

			comp.build_source = " {0:?} -o {1:?} ";
			comp.build_module = " -xc++ {0:?} -o {1:?} ";
			comp.build_prefix = "call {0:?} @{1}/SRC_INCLUDES -c -fPIC "
				// Fixes/hacks for pthread in gcc
				"-DWINPTHREAD_CLOCK_DECL=WINPTHREADS_ALWAYS_INLINE "
				"-DWINPTHREAD_COND_DECL=WINPTHREADS_ALWAYS_INLINE "
				"-DWINPTHREAD_MUTEX_DECL=WINPTHREADS_ALWAYS_INLINE "
				"-DWINPTHREAD_NANOSLEEP_DECL=WINPTHREADS_ALWAYS_INLINE "
				"-DWINPTHREAD_RWLOCK_DECL=WINPTHREADS_ALWAYS_INLINE "
				"-DWINPTHREAD_SEM_DECL=WINPTHREADS_ALWAYS_INLINE "
				"-DWINPTHREAD_THREAD_DECL=WINPTHREADS_ALWAYS_INLINE "
				;
			comp.link_libraries = " -lstdc++exp";
			comp.windows_link_libraries = " -lws2_32 -lwinhttp -lhttpapi";
#ifdef _MSC_VER
			comp.link_command = "call {0:?} -static -Wl,--allow-multiple-definition -o {1}/{2}";
			comp.dlib_command = "call {0:?} -shared -Wl,--out-implib,{1}/{3} -o {1}/{2}";
#else
			comp.link_command = "call {0:?} -static -o {1}/{2}";
			comp.dlib_command = "call {0:?} -shared -o {1}/{2}";
#endif
			comp.slib_command = "call {0:?} rcs {1}/{2}";
			comp.define = "-D";
			comp.include = "-I{0}";
			comp.module_path = " -fmodule-mapper=\"|@g++-mapper-server --root {}\"";
			callback(std::move(comp));
		}
	}
}
