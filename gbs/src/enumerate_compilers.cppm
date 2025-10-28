export module enumerate_compilers;
import std;
import compiler;
import env;
import enumerate_compilers_msvc;

export void enumerate_compilers(environment const& env, auto&& callback) {
	std::string cmd, version;

#ifdef _MSC_VER
	// Look for installations of Visual Studio
	std::string msvc_std_module{};
	if (int const inst = std::system("\">instpath.txt \"%ProgramFiles(x86)%/Microsoft Visual Studio/Installer/vswhere.exe\" -prerelease -property installationPath 2>nul\""); inst == 0) {
		std::string line;
		std::ifstream file("instpath.txt");

		while (std::getline(file, line)) {
			std::filesystem::path const msvc_path(line);
			enumerate_compilers_msvc(msvc_path / "VC" / "Tools" / "MSVC", callback);
		}
		file.close();
		std::error_code ec;
		std::filesystem::remove("instpath.txt", ec);
	}

	// Look for user installations of Microsoft Build Tools
	if (auto const prg_86 = env.get("ProgramFiles(x86)"); prg_86) {
		auto const build_tools_dir = std::filesystem::path(*prg_86) / "Microsoft Visual Studio" / "2022" / "BuildTools";
		if (std::filesystem::exists(build_tools_dir)) {
			// Find msvc compilers
			auto const msvc_path = build_tools_dir / "VC" / "Tools" / "MSVC";
			if (std::filesystem::exists(msvc_path)) {
				enumerate_compilers_msvc(msvc_path, callback);
			}
		}
	}
#endif

	// Find the users folder
	auto const home_dir = env.get_home_dir();

	// Find compilers in ~/.gbs/*
	auto const download_dir = home_dir / ".gbs";

	if (std::filesystem::exists(download_dir)) {
		if (std::filesystem::exists(download_dir / "clang")) {
			for (auto const& dir : std::filesystem::directory_iterator(download_dir / "clang")) {
				auto const path = dir.path().filename().string();
				std::string_view clang_version = path;

				if (path.starts_with("clang_")) {
					compiler comp;

					// format: clang_19.1.7
					comp.name_and_version = clang_version;
					clang_version.remove_prefix(6); // remove 'clang_'

					extract_compiler_version(clang_version, comp.major, comp.minor, comp.patch);
					comp.name = "clang";
					comp.arch = "x64";
					comp.dir = dir;
					comp.executable = dir.path() / "bin" / "clang";
					comp.linker = comp.executable;
					comp.slib = dir.path() / "bin" / "llvm-ar";
					comp.dlib = comp.executable;

					comp.build_source = " {0:?} -o {1:?} ";
					comp.build_module = " --language=c++-module {0:?} -o {1:?} -fmodule-output ";
					comp.build_command_prefix = "call {0:?} @{1}/SRC_INCLUDES -c ";
					comp.link_command = "call {0:?} -o {1}/{2}.exe @{1}/OBJLIST @{1}/LIBLIST";
					comp.slib_command = "call {0:?} rcs {1}/{2}.lib @{1}/OBJLIST";
					comp.dlib_command = "call {0:?} -shared -o {1}/{2}.dll @{1}/OBJLIST";
					comp.define = "-D";
					comp.include = "-I\"{0}/\"";
					comp.reference = " -fmodule-file={}={}.pcm ";
					callback(std::move(comp));
				}
			}
		}

		if (std::filesystem::exists(download_dir / "gcc")) {
			for (auto const& dir : std::filesystem::directory_iterator(download_dir / "gcc")) {
				auto const path = dir.path().filename().string();
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
					comp.arch = "x64";
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
					comp.link_command = "call {0:?} -o {1}/{2}.exe @{1}/OBJLIST @{1}/LIBLIST -static -Wl,--allow-multiple-definition -lstdc++exp";
#else
					comp.link_command = "call {0:?} -o {1}/{2}.exe @{1}/OBJLIST @{1}/LIBLIST -static";
#endif
					comp.dlib_command = "call {0:?} -o {1}/{2}.dll @{1}/OBJLIST -static -shared -Wl,--out-implib={1}/{2}.lib -lstdc++exp";
					comp.slib_command = "call {0:?} rcs {1}/{2}.lib @{1}/OBJLIST";
					comp.define = "-D";
					comp.include = "-I{0}";
					comp.reference = "";
					callback(std::move(comp));
				}
			}
		}
	}
}
