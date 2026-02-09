module;
#include <filesystem>
#include <iostream>
#include <print>
export module enumerate_compilers_gcc;
import env;
import compiler;

export void enumerate_compilers_gcc(environment const& env, auto&& callback) {
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
				// /usr/include/c++/15/bits/std.cc
				comp.std_module = actual_path / "include" / "c++" / comp.name_and_version.substr(4) / "bits" / "std.cc";
				if (!std::filesystem::exists(*comp.std_module)) {
					std::println(std::cerr, "<gbs> Error: Could not find 'std.cc' at location {}", comp.std_module->generic_string());
					std::println(std::cerr, "<gbs>        `import std;` will not be available.");
					comp.std_module.reset();
				}
			}

			comp.build_source = " {0:?} -o {1:?} ";
			comp.build_module = " -xc++ {0:?} -o {1:?} ";
			comp.build_command_prefix = "call {0:?} @{1}/SRC_INCLUDES -c -fPIC "
				// Fixes/hacks for pthread in gcc
				"-DWINPTHREAD_CLOCK_DECL=WINPTHREADS_ALWAYS_INLINE "
				"-DWINPTHREAD_COND_DECL=WINPTHREADS_ALWAYS_INLINE "
				"-DWINPTHREAD_MUTEX_DECL=WINPTHREADS_ALWAYS_INLINE "
				"-DWINPTHREAD_NANOSLEEP_DECL=WINPTHREADS_ALWAYS_INLINE "
				"-DWINPTHREAD_RWLOCK_DECL=WINPTHREADS_ALWAYS_INLINE "
				"-DWINPTHREAD_SEM_DECL=WINPTHREADS_ALWAYS_INLINE "
				"-DWINPTHREAD_THREAD_DECL=WINPTHREADS_ALWAYS_INLINE "
				;
#ifdef _MSC_VER
			comp.link_command = "call {0:?} -o {1}/{2} @{1}/OBJLIST @{1}/LIBLIST -static -Wl,--allow-multiple-definition -lstdc++exp";
#else
			comp.link_command = "call {0:?} -o {1}/{2} @{1}/OBJLIST @{1}/LIBLIST -static";
#endif
			comp.dlib_command = "call {0:?} -o {1}/{2} @{1}/OBJLIST -static -shared -Wl,--out-implib={1}/{2}.lib -lstdc++exp";
			comp.slib_command = "call {0:?} rcs {1}/{2} @{1}/OBJLIST";
			comp.define = "-D";
			comp.include = "-I{0}";
			comp.module_path = " -fmodule-mapper=\"|@g++-mapper-server --root {}\"";
			callback(std::move(comp));
		}
	}
}
