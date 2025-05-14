export module compiler;
import std;
import env;

char const* archs[] = { /*"arm64",*/ "x64" };

export struct compiler {
	int major = 0, minor = 0;
	std::string_view name;
	std::string_view arch;
	std::filesystem::path dir;
	std::filesystem::path exe;
};


export void extract_compiler_version(std::string_view sv, int& major, int& minor) {
	if (sv.contains('.')) {
		major = std::atoi(sv.substr(0, sv.find('.')).data());
		minor = std::atoi(sv.substr(sv.find('.') + 1).data());
	} else {
		major = std::atoi(sv.data());
		minor = 0;
	}
}

void enumerate_compilers_msvc(std::filesystem::path msvc_path, auto&& callback) {
	if (!std::filesystem::exists(msvc_path))
		return;

	for (auto const& dir : std::filesystem::directory_iterator(msvc_path)) {
		for (auto arch : archs) {
			compiler comp;
			comp.name = "msvc";
			comp.arch = arch;
			comp.dir = dir;
			comp.exe = comp.dir / "bin/HostX64" / arch;

			if (!std::filesystem::exists(comp.exe))
				continue;

			comp.exe /= "cl.exe";
			std::string const cmd = std::format(R"("{}" 1>nul 2>version)", comp.exe.generic_string());
			if (0 == std::system(cmd.c_str())) {
				std::string version;
				std::getline(std::ifstream("version"), version);

				std::string_view sv(version);
				sv.remove_prefix(sv.find_first_of("0123456789", 0));
				sv = sv.substr(0, sv.find_first_of(' '));

				extract_compiler_version(sv, comp.major, comp.minor);
				callback(std::move(comp));

				std::filesystem::remove("version");
			}
		}
	}
}

void enumerate_compilers_clang_cl(std::filesystem::path llvm_path, auto&& callback) {
	if (!std::filesystem::exists(llvm_path))
		return;

	compiler comp;
	comp.name = "clang-cl";
	for (auto arch : archs) {
		comp.arch = arch;
		comp.dir = llvm_path / arch;
		comp.exe = llvm_path / arch / "bin";

		if (!std::filesystem::exists(comp.exe))
			continue;

		comp.exe += "\\clang-cl.exe";
		std::string const cmd = std::format(R"("{}" -v 2>version)", comp.exe.generic_string());
		if (0 == std::system(cmd.c_str())) {
			std::string version;
			std::getline(std::ifstream("version"), version);

			std::string_view sv(version);
			sv.remove_prefix(sv.find_first_of("0123456789", 0));

			comp.major = std::atoi(sv.substr(0, sv.find('.')).data());
			comp.minor = std::atoi(sv.substr(sv.find('.') + 1).data());
			callback(std::move(comp));

			std::error_code ec;
			std::filesystem::remove("version", ec);
		}
	}
}

export void enumerate_compilers(auto&& callback) {
	std::string line, cmd, version;
	compiler comp;

	// Look for installations of Visual Studio
	int const inst = std::system("\">instpath.txt \"%ProgramFiles(x86)%/Microsoft Visual Studio/Installer/vswhere.exe\" -prerelease -property installationPath 2>nul\"");
	if (inst == 0) {
		std::ifstream file("instpath.txt");

		while (std::getline(file, line)) {
			std::filesystem::path const path(line);
			enumerate_compilers_msvc(path / "VC/Tools/MSVC", callback);
			//enumerate_compilers_clang_cl(path / "VC/Tools/LLVM", callback);
		}
		file.close();
		std::error_code ec;
		std::filesystem::remove("instpath.txt", ec);
	}

	// Look for user installations of Microsoft Build Tools
	if (auto const prg_86 = get_env_value("ProgramFiles(x86)"); !prg_86.empty()) {
		auto const build_tools_dir = std::filesystem::path(prg_86) / "Microsoft Visual Studio" / "2022" / "BuildTools";
		if (std::filesystem::exists(build_tools_dir)) {
			// Find msvc compilers
			auto const msvc_path = build_tools_dir / "VC/Tools/MSVC";
			if (std::filesystem::exists(msvc_path)) {
				enumerate_compilers_msvc(msvc_path, callback);
			}
		}
	}


	// Find the users folder
	auto home_dir = get_home_dir();
	if (home_dir.empty()) {
		std::println("<gbs> get_cl : unable to get home directory");
		return;
	}

	// Find compilers in ~/.gbs/*
	auto const download_dir = std::filesystem::path(home_dir) / ".gbs";

	if (std::filesystem::exists(download_dir)) {
		if (std::filesystem::exists(download_dir / "clang")) {
			for (auto const& dir : std::filesystem::directory_iterator(download_dir / "clang")) {
				auto const path = dir.path().filename().string();
				std::string_view clang_version = path;

				if (path.starts_with("clang_")) {
					// format: clang_19.1.7
					clang_version.remove_prefix(6); // remove 'clang_'

					comp = {};
					extract_compiler_version(clang_version, comp.major, comp.minor);
					comp.name = "clang";
					comp.arch = "x64";
					comp.dir = dir;
					comp.exe = dir.path() / "bin/clang";
					callback(std::move(comp));
				}
			}
		}

		if (std::filesystem::exists(download_dir / "gcc")) {
			for (auto const& dir : std::filesystem::directory_iterator(download_dir / "gcc")) {
				auto const path = dir.path().filename().string();
				std::string_view gcc_version = path;

				if (path.starts_with("gcc_")) {
					// gcc_13.3.0-2
					gcc_version.remove_prefix(4);

					comp = {};
					extract_compiler_version(gcc_version, comp.major, comp.minor);
					comp.name = "gcc";
					comp.arch = "x64";
					comp.dir = dir;
					comp.exe = dir.path() / "bin/gcc";
					callback(std::move(comp));
				}
			}
		}

		if (std::filesystem::exists(download_dir / "msvc")) {
			enumerate_compilers_msvc(download_dir / "msvc/VC/Tools/MSVC", callback);
			//enumerate_compilers_clang_cl(download_dir / "msvc/VC/Tools/LLVM", callback);
		}
	}
}

