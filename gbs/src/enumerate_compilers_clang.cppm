export module enumerate_compilers_clang;
import std;
import env;
import compiler;

export void enumerate_compilers_clang(environment const& env, auto&& callback) {
	// Find compilers in ~/.gbs/clang
	std::filesystem::path const download_dir = env.get_home_dir() / ".gbs" / "clang";

	if (!std::filesystem::exists(download_dir))
		return;
	
	for (auto const& dir : std::filesystem::directory_iterator(download_dir)) {
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
