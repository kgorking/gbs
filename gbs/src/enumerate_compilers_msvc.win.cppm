export module enumerate_compilers_msvc;
import std;
import compiler;

export void enumerate_compilers_msvc(std::filesystem::path msvc_path, auto&& callback) {
	if (!std::filesystem::exists(msvc_path))
		return;

	for (auto const& dir : std::filesystem::directory_iterator(msvc_path)) {
		auto const arch = "x64";
		/*for (auto const arch : archs)*/ {
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
			comp.build_command_prefix = "call {0:?} @{1}/INCLUDE @{1}/SRC_INCLUDES /c /interface /TP /ifcOutput {1}/ /Fo:{1}/ ";
			comp.link_command = "call {0:?} /NOLOGO /OUT:{1}/{2}.exe @{1}/LIBPATH @{1}/OBJLIST @{1}/LIBLIST";
			comp.slib_command = "call {0:?} /NOLOGO /OUT:{1}/{2}.lib @{1}/LIBPATH @{1}/OBJLIST";
			comp.dlib_command = "call {0:?} /NOLOGO /DLL /OUT:{1}/{2}.dll @{1}/LIBPATH @{1}/OBJLIST";
			comp.define = "/D";
			comp.include = "/I{0}";
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
