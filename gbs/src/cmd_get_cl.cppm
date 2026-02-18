module;
#include <filesystem>
#include <fstream>
#include <print>
#include <string>
#include <string_view>
export module cmd_get_cl;
import compiler;
import context;
import env;

std::string gcc_get_download_url(std::string_view const version) {
	// Input:  15.2.0posix-13.0.0-msvcrt-r1
	// Output: https://github.com/brechtsanders/winlibs_mingw/releases/download/15.2.0posix-13.0.0-ucrt-r1/winlibs-x86_64-posix-seh-gcc-15.2.0-mingw-w64msvcrt-13.0.0-r1.zip
	//         https://github.com/brechtsanders/winlibs_mingw/releases/download/15.2.0posix-13.0.0-msvcrt-r1/winlibs-x86_64-posix-seh-gcc-15.2.0-mingw-w64msvcrt-13.0.0-r1.zip
	//         https://github.com/brechtsanders/winlibs_mingw/releases/download/16.0.0-snapshot20251026posix-14.0.0-ucrt-r1/winlibs-x86_64-posix-seh-gcc-16.0.0-snapshot20251026-mingw-w64ucrt-14.0.0-r1.zip
	//         https://github.com/brechtsanders/winlibs_mingw/releases/download/16.0.0-snapshot20251026posix-14.0.0-msvcrt-r1/winlibs-x86_64-posix-seh-gcc-16.0.0-snapshot20251026-mingw-w64msvcrt-14.0.0-r1.zip
	// 
	//static constexpr std::string_view template_url = "https://github.com/brechtsanders/winlibs_mingw/releases/download/{0}/winlibs-x86_64-posix-seh-gcc-{1}-mingw-w64ucrt-{2}-{3}.zip";
	static constexpr std::string_view template_url =   "https://github.com/brechtsanders/winlibs_mingw/releases/download/{0}/winlibs-x86_64-posix-seh-gcc-{1}-mingw-w64msvcrt-{2}-{3}.zip";

	std::size_t const posix_pos = version.find("posix-");
	std::string_view const gcc_ver = version.substr(0, posix_pos);

	std::string_view posix_ver = version.substr(posix_pos + 6);
	posix_ver = posix_ver.substr(0, posix_ver.find_first_of('-'));

	std::string_view const release_ver = version.substr(version.rfind('-') + 1);

	return std::vformat(template_url, std::make_format_args(version, gcc_ver, posix_ver, release_ver));
}

std::string clang_get_download_url(std::string_view const version) {
	// "https://github.com/llvm/llvm-project/releases/download/llvmorg-{0}/clang+llvm-{0}-x86_64-pc-windows-msvc.tar.xz";
	return std::format("https://github.com/llvm/llvm-project/releases/download/llvmorg-{0}/clang+llvm-{0}-x86_64-pc-windows-msvc.tar.xz", version);
}

export bool cmd_get_cl(context& ctx, std::string_view args) {
	std::println("<gbs> '{}' - Searching remotely for newest version...", args);

	if (ctx.set_compiler(args)) {
		return true;
	}

	if (args.empty()) {
		std::println("<gbs> Error : usage 'get_cl=compilername:version', eg. 'get_cl=clang:19.1'");
		return false;
	}

	if (1 == std::system("git --version >nul")) {
		std::println("<gbs> Error : 'git' is required for this command to work");
		return false;
	}

	if (1 == std::system("tar --version >nul")) {
		std::println("<gbs> Error : 'tar' is required for this command to work");
		return false;
	}

	if (1 == std::system("curl --version >nul")) {
		std::println("<gbs> Error : 'curl' is required for this command to work");
		return false;
	}

	// Find the users folder
	auto const homedir = ctx.get_home_dir();

	compiler cl;
	cl.name = args.substr(0, args.contains(':') ? args.find_first_of(':') : args.size());
	std::string_view version_prefix;
	std::string version;
	std::string git_search_cmd;
	std::string_view extract_output_dir;

	args.remove_prefix(cl.name.size());
	if (args.empty())
		args = ":";

	if (args.contains(':')) {
		args.remove_prefix(1);
		if (args.empty())
			args = "[0-9]";
			
		// Version to search for
		version = args;
	}

	std::string(*get_download_url)(std::string_view) = nullptr;

	if (cl.name == "clang") {
		cl.executable = "bin/clang";

		version_prefix = "refs/tags/llvmorg-";

		// include a dot to avoid -init tags
		std::string_view const has_dot_sv = version.contains('.') ? "" : ".";

		git_search_cmd = std::format("git ls-remote --exit-code --refs --tags --sort=\"-version:refname\" https://github.com/llvm/llvm-project *{}{}* > {}",
			version,
			has_dot_sv,
			"version_list.txt");

		// Prepare the download url
		get_download_url = clang_get_download_url;
		extract_output_dir = "clang-{0}-{1}";
	}
	else if (cl.name == "gcc") {
		// git ls-remote --exit-code --refs --tags --sort="-version:refname" https://github.com/xpack-dev-tools/gcc-xpack v*
		// https://github.com/xpack-dev-tools/gcc-xpack/releases/download/v14.2.0-2/xpack-gcc-14.2.0-2-win32-x64.zip
		// https://github.com/brechtsanders/winlibs_mingw/releases/tag/15.2.0posix-13.0.0-msvcrt-r1
		// download/15.2.0posix-13.0.0-msvcrt-r1/winlibs-x86_64-posix-seh-gcc-15.2.0-mingw-w64msvcrt-13.0.0-r1.zip
		cl.executable = "mingw64/bin/gcc";
		version_prefix = "refs/tags/";

		git_search_cmd = std::format("git ls-remote --exit-code --refs --tags --sort=\"-version:refname\" https://github.com/brechtsanders/winlibs_mingw *{}*posix*msvcrt* > {}",
			version,
			"version_list.txt");

		get_download_url = gcc_get_download_url;
		extract_output_dir = "gcc-{0}_{1}";
	}
	else {
		std::println("<gbs>    Unsupported compiler {}", cl.name);
		return false;
	}

	// Make sure the download function is set
	if (nullptr == get_download_url) {
		std::println("<gbs>    Internal error, no download function for {}", cl.name);
		return false;
	}

	// Search for compiler version
	if (2 == std::system(git_search_cmd.c_str())) {
		std::filesystem::remove("version_list.txt");
		std::println("<gbs>    no valid version found");
		return false;
	}

	// Load the closest version from the version file
	std::string url;
	{
		std::ifstream versions("version_list.txt");
		bool found_valid = false;
		do {
			if (!(versions >> version))
				break;
			versions >> version;

			// Strip down to version
			version = version.substr(version_prefix.size());

			// Build url
			url = get_download_url(version);

			// Check for file
			found_valid = (22 != std::system(std::format("curl -fsI {} >nul", url).c_str()));
			if (found_valid) {
				extract_compiler_version(version, cl.major, cl.minor, cl.patch);
				std::println("<gbs>    Found {} {}.{}.{}", cl.name, cl.major, cl.minor, cl.patch);
			}
		} while (!found_valid);

		if (!found_valid) {
			std::println("<gbs>    No suitable download found for {}:{}", cl.name, args);
			return false;
		} 
	}
	std::filesystem::remove("version_list.txt");


	std::string_view const filename = std::string_view{ url }.substr(1 + url.find_last_of('/'));
	auto const gbs_user_path = std::filesystem::path(homedir) / ".gbs";
	auto const downloaded_file_path = gbs_user_path / filename;
	auto const compiler_dir = gbs_user_path / cl.name;
	auto const download_dir = compiler_dir / std::vformat(extract_output_dir, std::make_format_args(version, "x64"));
	auto const dest_dir = compiler_dir / std::format("{}_{}.{}.{}", cl.name, cl.major, cl. minor, cl.patch);

	// Check if already downloaded
	cl.dir = dest_dir;
	cl.executable = cl.dir / cl.executable; // update exe path

	if (std::filesystem::exists(cl.dir)) {
		// TODO check bin

		std::println("<gbs>    Skipping download, found in {}", cl.dir.generic_string());
		return true;
	}

	// Prep the destination dir
	std::filesystem::create_directories(dest_dir);

	// Download
	std::println("<gbs>    {}", url);
	if (0 != std::system(std::format("curl -fSL {} | tar --strip-components=1 -xf - -C {}", url, dest_dir.generic_string()).c_str())) {
		std::println("<gbs> Error downloading and unpacking {} {}", cl.name, version);
		return false;
	}

	// Refill compiler collection
	ctx.fill_compiler_collection();
	ctx.set_compiler(args);

	std::println("<gbs> Download successful");
	return true;
}