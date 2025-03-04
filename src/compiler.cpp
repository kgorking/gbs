#include <ranges>
#include <string>
#include <print>
#include <fstream>
#include "compiler.h"
#include "context.h"

char const* archs[] = { /*"arm64",*/ "x64" };

bool is_file_up_to_date(std::filesystem::path const& in, std::filesystem::path const& out) {
	return std::filesystem::exists(out) && (std::filesystem::last_write_time(out) > std::filesystem::last_write_time(in));
}

void extract_version(std::string_view sv, int& major, int& minor) {
	major = std::atoi(sv.substr(0, sv.find('.')).data());
	minor = std::atoi(sv.substr(sv.find('.') + 1).data());
}

void enumerate_compilers(auto&& callback) {
	std::string line, cmd, version;
	compiler comp;

	int const inst = std::system("\">instpath.txt \"%ProgramFiles(x86)%/Microsoft Visual Studio/Installer/vswhere.exe\" -prerelease -property installationPath 2>nul\"");
	if (inst == 0) {
		std::ifstream file("instpath.txt");

		while (std::getline(file, line)) {
			std::filesystem::path const path(line);

			// Find msvc compilers
			auto const msvc_path = path / "VC\\Tools\\MSVC";
			if (std::filesystem::exists(msvc_path)) {
				comp.name = "msvc";

				for (auto const& dir : std::filesystem::directory_iterator(msvc_path)) {
					for (auto arch : archs) {
						comp = {};
						comp.arch = arch;
						comp.dir = dir;
						comp.exe = std::format("{}\\bin\\HostX64\\{}", dir.path().string(), arch);
						comp.inc = std::format("{}\\include", dir.path().string());
						comp.lib = std::format("{}\\lib\\{}", dir.path().string(), arch);

						if (!std::filesystem::exists(comp.exe))
							continue;

						comp.exe += "\\cl.exe";
						cmd = std::format(R"("{}" 1>nul 2>version)", comp.exe.generic_string());
						if (0 == std::system(cmd.c_str())) {
							std::getline(std::ifstream("version"), version);

							std::string_view sv(version);
							sv.remove_prefix(sv.find_first_of("0123456789", 0));
							sv = sv.substr(0, sv.find_first_of(' '));

							extract_version(sv, comp.major, comp.minor);
							callback(std::move(comp));
						}
					}
				}
			}

			// Find clang-cl compilers
			auto const llvm_path = path / "VC\\Tools\\LLVM";
			if (std::filesystem::exists(llvm_path)) {
				comp.name = "clang-cl";
				for (auto arch : archs) {
					comp.arch = arch;
					comp.dir = llvm_path / arch;
					comp.exe = std::format("{}\\{}\\bin", llvm_path.string(), arch);

					if (!std::filesystem::exists(comp.exe))
						continue;

					comp.exe += "\\clang-cl.exe";
					cmd = std::format(R"("{}" -v 2>version)", comp.exe.generic_string());
					if (0 == std::system(cmd.c_str())) {
						std::getline(std::ifstream("version"), version);

						std::string_view sv(version);
						sv.remove_prefix(sv.find_first_of("0123456789", 0));

						comp.major = std::atoi(sv.substr(0, sv.find('.')).data());
						comp.minor = std::atoi(sv.substr(sv.find('.') + 1).data());
						callback(std::move(comp));
					}
				}
			}
		}
		file.close();
		std::error_code ec;
		std::filesystem::remove("instpath.txt", ec);
	}

	// Find clang compilers
	// TODO test

	std::error_code ec;
	std::filesystem::remove("version", ec);
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


compiler get_compiler(context const& ctx, std::string_view comp) {
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
		std::println("Error: ill-formed compiler descriptor: {}", comp);
		exit(1);
	}


	if (!ctx.all_compilers.contains(cl)) {
		std::println("Unknown compiler: {}", cl);
		exit(1);
	}

	// Select the compiler
	auto const& named_compilers = ctx.all_compilers.at(cl);
	if (version.empty())
		return named_compilers.front();

	// Select the version
	int const major = std::atoi(version.substr(0, version.find('.')).data());
	int const minor = std::atoi(version.substr(version.find('.') + 1).data());

	auto version_compilers = named_compilers | std::views::filter([major, minor](compiler const& c) {
		if (c.major == major)
			return c.minor >= minor;
		return c.major >= major;
		});

	if (version_compilers.empty()) {
		std::println("Error: A compiler with a higher version than what is available was requested");
		exit(1);
	}

	if (arch.empty())
		return version_compilers.front();

	// Select architecture
	auto arch_compilers = version_compilers | std::views::filter([arch](compiler const& c) {
		return arch == c.arch;
		});

	if (arch_compilers.empty()) {
		std::println("Error: A compiler with architecture '{}' was not found.", arch);
		exit(1);
	}

	return arch_compilers.front();
}
