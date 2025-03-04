#include <string_view>
#include <fstream>
#include <print>
#include "context.h"
#include "home.h"

bool get_cl(context& ctx, std::string_view args) {
	if (args.empty()) {
		std::println("<gbs> get_cl error : usage 'get_cl=compilername:version', eg. 'get_cl=clang:19.1'");
		return false;
	}

	if (1 == std::system("git --version >nul")) {
		std::println("<gbs> get_cl : 'git' is required for this command to work");
		return false;
	}


	if (1 == std::system("tar --version >nul")) {
		std::println("<gbs> get_cl : 'tar' is required for this command to work");
		return false;
	}

	// Find the users folder
	char const* homedir = get_home_dir();
	if (!homedir) {
		std::println("<gbs> get_cl : unable to get home directory");
		return false;
	}

	std::println("<gbs> get_cl : searching for {}", args);

	std::string_view compiler_name = args.substr(0, args.contains(':') ? args.find_first_of(':') : args.size());
	std::string_view version_prefix;
	std::string version;
	std::string git_search_cmd;
	std::string_view gh_download_url;
	std::string_view extract_output_dir;

	args.remove_prefix(compiler_name.size());

	if (compiler_name == "clang") {
		version_prefix = "refs/tags/llvmorg-";
		git_search_cmd = "";

		if (args.empty())
			args = ":";

		if (args.contains(':')) {
			args.remove_prefix(1);
			if (args.empty())
				args = "[0-9]";
			
			// Version to search for
			version = args;
		}

		// include a dot to avoid -init tags
		std::string_view const has_dot_sv = version.contains('.') ? "" : ".";

		git_search_cmd = std::format("git ls-remote --exit-code --refs --tags --sort=\"-version:refname\" https://github.com/llvm/llvm-project *{}{}* > {}",
			version,
			has_dot_sv,
			"version_list.txt");

		// Prepare the download url
#ifdef _WIN64
		gh_download_url = "https://github.com/llvm/llvm-project/releases/download/llvmorg-{0}/clang+llvm-{0}-x86_64-pc-windows-msvc.tar.xz";
		extract_output_dir = "clang+llvm-{0}-x86_64-pc-windows-msvc";
#elif __linux__
		gh_download_url = "https://github.com/llvm/llvm-project/releases/download/llvmorg-{0}/clang+llvm-{0}-aarch64-linux-gnu.tar.xz";
		extract_output_dir = "clang+llvm-{0}-aarch64-linux-gnu";
#elif _ARM // ??
		gh_download_url = "https://github.com/llvm/llvm-project/releases/download/llvmorg-{0}/clang+llvm-{0}-armv7a-linux-gnueabihf.tar.gz";
		extract_output_dir = "clang+llvm-{0}-armv7a-linux-gnueabihf";
#endif
	}
	else if (compiler_name == "gcc") {
		// git ls-remote --exit-code --refs --tags --sort="-version:refname" https://github.com/xpack-dev-tools/gcc-xpack v*
		// https://github.com/xpack-dev-tools/gcc-xpack/releases/download/v14.2.0-2/xpack-gcc-14.2.0-2-win32-x64.zip
	}
	else {
		std::println("<gbs> get_cl : only clang downloads supported so far");
		return false;
	}

	// Search for compiler version
	if (2 == std::system(git_search_cmd.c_str())) {
		std::remove("version_list.txt");
		std::println("<gbs> get_cl :   no valid version found");
		return false;
	}

	// Load the closest version from the version file
	{
		std::ifstream versions("version_list.txt");
		versions >> version;
		versions >> version;
	}
	std::remove("version_list.txt");

	// Strip down to version
	version = version.substr(version_prefix.size());

	// Check if already downloaded
	std::string const url = std::vformat(gh_download_url, std::make_format_args(version));
	std::string const dirname = std::vformat(extract_output_dir, std::make_format_args(version));

	if (std::filesystem::exists(std::filesystem::path(homedir) / ".gbs" / dirname)) {
		std::println("<gbs> get_cl : already downloaded {} version {}", compiler_name, version);
		return true;
	}

	// Download
	std::println("<gbs> get_cl :   downloading version {}", version);
	if (0 != std::system(std::format("curl -L {} | tar xz -C \"{}/.gbs\"", url, homedir).c_str())) {
		std::println("<gbs> get_cl : error downloading {} version {}", compiler_name, version);
		return false;
	}

	// Add it to the list of supported compilers
	//ctx.all_compilers["clang"].emplace_back();

	std::println("<gbs> get_cl : download successful");
	return false;
}