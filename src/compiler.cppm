export module compiler;
import std;
import env;

constexpr std::array<std::string_view, 1> archs = { /*"arm64",*/ "x64" };

export struct compiler {
	int major = 0, minor = 0, patch = 0;
	std::string name_and_version;
	std::string_view name;
	std::string_view arch;
	std::string_view build_source;
	std::string_view build_module;
	std::string_view build_command_prefix;
	std::string_view link_command;
	std::string_view slib_command;
	std::string_view dlib_command;
	std::string_view reference;
	std::filesystem::path dir;
	std::filesystem::path executable;
	std::filesystem::path linker;
	std::filesystem::path slib;
	std::filesystem::path dlib;
	std::optional<std::filesystem::path> std_module;
};


export void extract_compiler_version(std::string_view sv, int& major, int& minor, int& patch) {
	major = 0;
	minor = 0;
	patch = 0;

	auto dot = sv.find('.');
	major = std::atoi(sv.substr(0, dot).data());
	if (dot == std::string_view::npos)
		return;

	sv.remove_prefix(dot + 1);
	dot = sv.find('.');
	minor = std::atoi(sv.data());
	if (dot == std::string_view::npos)
		return;

	sv.remove_prefix(dot + 1);
	dot = sv.find('.');
	patch = std::atoi(sv.data());
}

void enumerate_compilers_msvc(std::filesystem::path msvc_path, auto&& callback) {
	if (!std::filesystem::exists(msvc_path))
		return;

	for (auto const& dir : std::filesystem::directory_iterator(msvc_path)) {
		for (auto const arch : archs) {
			compiler comp;
			comp.name = "msvc";
			comp.name_and_version = "msvc_" + dir.path().filename().string();
			comp.arch = arch;
			comp.dir = dir;
			comp.executable = comp.dir / "bin" / "HostX64" / arch / "cl.exe";
			comp.linker = comp.dir / "bin" / "HostX64" / arch / "link.exe";
			comp.slib = comp.dir / "bin" / "HostX64" / arch / "lib.exe";
			comp.dlib = comp.linker;
			comp.std_module = comp.dir / "modules" / "std.ixx";

			comp.build_source = " {0:?} ";
			comp.build_module = " {0:?} ";
			comp.build_command_prefix = "call {0:?} @{1}/INCLUDE /c /interface /TP /ifcOutput {1}/ /Fo:{1}/";
			comp.link_command = "call {0:?} /NOLOGO /OUT:{1}/{2}.exe @{1}/LIBPATH @{1}/OBJLIST";
			comp.slib_command = "call {0:?} /NOLOGO /OUT:{1}/{2}.lib @{1}/LIBPATH @{1}/OBJLIST";
			comp.dlib_command = "call {0:?} /NOLOGO /DLL /OUT:{1}/{2}.dll @{1}/LIBPATH @{1}/OBJLIST >nul";
			comp.reference = " /reference {0}={1}.ifc ";

			if (!std::filesystem::exists(comp.executable))
				continue;

			std::string const cmd = std::format(R"("{}" 1>nul 2>version)", comp.executable.generic_string());
			if (0 == std::system(cmd.c_str())) {
				std::string version;
				std::getline(std::ifstream("version"), version);

				std::string_view sv(version);
				sv.remove_prefix(sv.find_first_of("0123456789", 0));
				sv = sv.substr(0, sv.find_first_of(' '));

				extract_compiler_version(sv, comp.major, comp.minor, comp.patch);
				callback(std::move(comp));

				std::filesystem::remove("version");
			}
		}
	}
}

export void enumerate_compilers(environment const& env, auto&& callback) {
	std::string line, cmd, version;

#ifdef _MSC_VER
	// Look for installations of Visual Studio
	std::string msvc_std_module{};
	int const inst = std::system("\">instpath.txt \"%ProgramFiles(x86)%/Microsoft Visual Studio/Installer/vswhere.exe\" -prerelease -property installationPath 2>nul\"");
	if (inst == 0) {
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
					comp.build_command_prefix = "call \"{0}\" -c ";
					comp.link_command = "call {0:?} @{1}/OBJLIST -o {1}/{2}.exe";
					comp.slib_command = "call {0:?} rcs {1}/{2}.lib @{1}/OBJLIST";
					comp.dlib_command = "call {0:?} -shared -o {1}/{2}.dll @{1}/OBJLIST";
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

					// gcc_13.3.0-2
					comp.name_and_version = gcc_version;
					gcc_version.remove_prefix(4);

					comp.executable = dir.path();
					if (std::filesystem::exists(dir.path() / "mingw64"))
						comp.executable /= "mingw64";

					extract_compiler_version(gcc_version, comp.major, comp.minor, comp.patch);
					comp.name = "gcc";
					comp.arch = "x64";
					comp.dir = dir;
					comp.executable = comp.executable / "bin" / "gcc";
					comp.linker = comp.executable;
					if (comp.major >= 15)
						comp.std_module = "bits/std.cc"; //

					comp.build_source = " {0:?} -o {1:?} ";
					comp.build_module = " -xc++ {0:?} -o {1:?} ";
					comp.build_command_prefix = "call \"{0}\" -c -fsearch-include-path ";
					comp.link_command = "call \"{0}\"  @{1}/OBJLIST -o {1}/{2}.exe";
					comp.reference = "";
					callback(std::move(comp));
				}
			}
		}
	}
}
