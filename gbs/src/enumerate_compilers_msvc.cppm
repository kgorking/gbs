module;
#include <filesystem>
export module enumerate_compilers_msvc;
import env;
import compiler;

#ifndef _MSC_VER
export void enumerate_compilers_msvc(environment const&, auto&&) {}
#else
static void enumerate_compiler_msvc(std::filesystem::path msvc_path, auto&& callback) {
	if (!std::filesystem::exists(msvc_path))
		return;

	for (auto const& dir : std::filesystem::directory_iterator(msvc_path)) {
		compiler comp;
		comp.name = "msvc";
		comp.name_and_version = "msvc_" + dir.path().filename().generic_string();
		comp.dir = dir;
		comp.executable = comp.dir / "bin" / "HostX64" / "x64" / "cl.exe";
		comp.linker = comp.dir / "bin" / "HostX64" / "x64" / "link.exe";
		comp.slib = comp.dir / "bin" / "HostX64" / "x64" / "lib.exe";
		comp.dlib = comp.linker;
		comp.std_module = comp.dir / "modules" / "std.ixx";

		comp.build_source = " {0:?} ";
		comp.build_module = " {0:?} ";
		comp.build_command_prefix = "call {0:?} @{1}/INCLUDE @{1}/SRC_INCLUDES /c /interface /TP /ifcOutput {1}/ /Fo:{1}/ ";
		comp.link_command = "call {0:?} /NOLOGO /OUT:{1}/{2}.exe @{1}/LIBPATH @{1}/OBJLIST @{1}/LIBLIST";
		comp.slib_command = "call {0:?} /NOLOGO /OUT:{1}/{2}.lib @{1}/LIBPATH @{1}/OBJLIST";
		comp.dlib_command = "call {0:?} /NOLOGO /DLL /OUT:{1}/{2}.dll @{1}/LIBPATH @{1}/OBJLIST";
		comp.define = "/D";
		comp.include = "/I{0}";
		comp.reference = " /reference {0}={1}.ifc ";
		comp.target = " /arch:{} ";

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

export void enumerate_compilers_msvc(environment const& env, auto&& callback) {
	// Look for installations of Visual Studio
	std::string msvc_std_module{};
	if (int const inst = std::system("\">instpath.txt \"%ProgramFiles(x86)%/Microsoft Visual Studio/Installer/vswhere.exe\" -prerelease -property installationPath 2>nul\""); inst == 0) {
		std::string line;
		std::ifstream file("instpath.txt");

		while (std::getline(file, line)) {
			std::filesystem::path const msvc_path(line);
			enumerate_compiler_msvc(msvc_path / "VC" / "Tools" / "MSVC", callback);
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
				enumerate_compiler_msvc(msvc_path, callback);
			}
		}
	}
}
#endif
