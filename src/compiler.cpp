#include <ranges>
#include <string>
#include <print>
#include <fstream>
#include "compiler.h"
#include "context.h"
#include "home.h"

char const* archs[] = { /*"arm64",*/ "x64" };

bool is_file_up_to_date(std::filesystem::path const& in, std::filesystem::path const& out) {
	return std::filesystem::exists(out) && (std::filesystem::last_write_time(out) > std::filesystem::last_write_time(in));
}

void extract_version(std::string_view sv, int& major, int& minor) {
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
				std::filesystem::remove("version");

				std::string_view sv(version);
				sv.remove_prefix(sv.find_first_of("0123456789", 0));
				sv = sv.substr(0, sv.find_first_of(' '));

				extract_version(sv, comp.major, comp.minor);
				callback(std::move(comp));
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

void enumerate_compilers(auto&& callback) {
	std::string line, cmd, version;
	compiler comp;

	// Look for installations of Visual Studio
	int const inst = std::system("\">instpath.txt \"%ProgramFiles(x86)%/Microsoft Visual Studio/Installer/vswhere.exe\" -prerelease -property installationPath 2>nul\"");
	if (inst == 0) {
		std::ifstream file("instpath.txt");

		while (std::getline(file, line)) {
			std::filesystem::path const path(line);
			enumerate_compilers_msvc(path / "VC/Tools/MSVC", callback);
			enumerate_compilers_clang_cl(path / "VC/Tools/LLVM", callback);
		}
		file.close();
		std::error_code ec;
		std::filesystem::remove("instpath.txt", ec);
	}

	// Look for user installations of Microsoft Build Tools
	if (auto const prg_86 = getenv("ProgramFiles(x86)"); prg_86) {
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
	char const* home_dir = get_home_dir();
	if (!home_dir) {
		std::println("<gbs> get_cl : unable to get home directory");
		return;
	}

	// Find compilers in ~/.gbs/*
	auto const download_dir = std::filesystem::path(home_dir) / ".gbs";
	for (auto const& dir : std::filesystem::directory_iterator(download_dir / "clang")) {
		auto const path = dir.path().filename().string();
		std::string_view version = path;

		if (path.starts_with("clang_")) {
			// format: clang_19.1.7
			version.remove_prefix(6); // remove 'clang_'

			comp = {};
			extract_version(version, comp.major, comp.minor);
			comp.name = "clang";
			comp.arch = "x64";
			comp.dir = dir;
			comp.exe = dir.path() / "bin/clang";
			callback(std::move(comp));
		}
	}

	for (auto const& dir : std::filesystem::directory_iterator(download_dir / "gcc")) {
		auto const path = dir.path().filename().string();
		std::string_view version = path;

		if(path.starts_with("gcc_")) {
			// gcc_13.3.0-2
			version.remove_prefix(4);

			comp = {};
			extract_version(version, comp.major, comp.minor);
			comp.name = "gcc";
			comp.arch = "x64";
			comp.dir = dir;
			comp.exe = dir.path() / "bin/gcc";
			callback(std::move(comp));
		}
	}

	if (std::filesystem::exists(download_dir / "msvc")) {
		enumerate_compilers_msvc(download_dir / "msvc/VC/Tools/MSVC", callback);
		enumerate_compilers_clang_cl(download_dir / "msvc/VC/Tools/LLVM", callback);
	}
}


void fill_compiler_collection(context& ctx) {
	ctx.all_compilers.clear();
	enumerate_compilers([&](compiler&& c) {
		ctx.all_compilers[c.name].push_back(std::forward<compiler>(c));
	});

	// Sort compilers from highest version to lowest
	for (auto& [k, v] : ctx.all_compilers) {
		std::ranges::sort(v, [](compiler const& c1, compiler const& c2) {
			if (c1.major == c2.major)
				return c1.minor > c2.minor;
			else
				return c1.major > c2.major;
			});
	}
}


std::optional<compiler> get_compiler(context const& ctx, std::string_view comp) {
	auto split = comp | std::views::split(':'); // cl:version:arch
	std::string_view cl, version, arch;

	switch (std::ranges::distance(split)) {
	case 1:
		// No version requested, returns newest
		cl = comp;
		break;

	case 2:
		// Version requested
		cl = std::string_view{ *split.begin() };
		version = std::string_view{ *std::next(split.begin()) };
		break;

	case 3:
		// Version and architecture requested
		cl = std::string_view{ *split.begin() };
		version = std::string_view{ *std::next(split.begin()) };
		arch = std::string_view{ *std::next(split.begin(), 2) };
		break;

	default:
		std::println("<gbs>   Error: ill-formed compiler descriptor: {}", comp);
		exit(1);
	}


	if (!ctx.all_compilers.contains(cl)) {
		return {};
	}

	// Select the compiler
	auto const& named_compilers = ctx.all_compilers.at(cl);
	if (version.empty())
		return named_compilers.front();

	// Select the version
	int major = 0, minor = 0;
	extract_version(version, major, minor);

	auto version_compilers = named_compilers | std::views::filter([major, minor](compiler const& c) {
		if (c.major == major)
			return c.minor >= minor;
		return false;// c.major >= major;
		});

	if (version_compilers.empty()) {
		std::println("<gbs>   Error: requested version not found");
		return {};
	}

	if (arch.empty())
		return version_compilers.front();

	// Select architecture
	auto arch_compilers = version_compilers | std::views::filter([arch](compiler const& c) {
		return arch == c.arch;
		});

	if (arch_compilers.empty()) {
		std::println("<gbs>   Error: A compiler with architecture '{}' was not found.", arch);
		return {};
	}

	return arch_compilers.front();
}
