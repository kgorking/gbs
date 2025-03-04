#include <string_view>
#include <fstream>
#include <print>
#include "context.h"
#include "home.h"

bool get_cl(context& ctx, std::string_view args) {
	std::println("<gbs> get_cl : searching for {}", args);

	if (std::optional<compiler> opt_cl = get_compiler(ctx, args); opt_cl) {
		ctx.selected_cl = *opt_cl;
		return true;
	}

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

	compiler cl;
	cl.name = args.substr(0, args.contains(':') ? args.find_first_of(':') : args.size());
	std::string_view version_prefix;
	std::string version;
	std::string git_search_cmd;
	std::string_view gh_download_url;
	std::string_view extract_output_dir;

	args.remove_prefix(cl.name.size());

	if (cl.name == "clang") {
		cl.exe = "bin/clang";

		version_prefix = "refs/tags/llvmorg-";

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
		cl.arch = "x64";
		gh_download_url = "https://github.com/llvm/llvm-project/releases/download/llvmorg-{0}/clang+llvm-{0}-x86_64-pc-windows-msvc.tar.xz";
		extract_output_dir = "clang+llvm-{0}-x86_64-pc-windows-msvc";
#elif __linux__
		cl.arch = "x64";
		gh_download_url = "https://github.com/llvm/llvm-project/releases/download/llvmorg-{0}/clang+llvm-{0}-aarch64-linux-gnu.tar.xz";
		extract_output_dir = "clang+llvm-{0}-aarch64-linux-gnu";
#elif _ARM // ??
		cl.arch = "arm64";
		gh_download_url = "https://github.com/llvm/llvm-project/releases/download/llvmorg-{0}/clang+llvm-{0}-armv7a-linux-gnueabihf.tar.gz";
		extract_output_dir = "clang+llvm-{0}-armv7a-linux-gnueabihf";
#endif
	}
	else if (cl.name == "gcc") {
		cl.exe = "bin/gcc";
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
	extract_version(version, cl.major, cl.minor);

	// Check if already downloaded
	std::string const url = std::vformat(gh_download_url, std::make_format_args(version));
	std::string const dirname = std::vformat(extract_output_dir, std::make_format_args(version));
	cl.dir = std::filesystem::path(homedir) / ".gbs" / dirname;

	if (std::filesystem::exists(cl.dir)) {
		// TODO check bin

		std::println("<gbs> get_cl : already downloaded {} version {}", cl.name, version);
		return true;
	}

	// Download and unpack
	std::println("<gbs> get_cl :   downloading {} version {}", cl.name, version);
	std::println("<gbs> get_cl :   {}", url);
	if (0 != std::system(std::format("curl -L {} | tar xz -C \"{}/.gbs\"", url, homedir).c_str())) {
		std::println("<gbs> get_cl : error downloading {} version {}", cl.name, version);
		return false;
	}

	//cl.inc = cl.dir / "include";
	//cl.lib = cl.dir / "lib";

	// Add it to the list of supported compilers
	ctx.all_compilers[cl.name].emplace_back(cl);
	ctx.selected_cl = cl;

	std::println("<gbs> get_cl : download successful");
	return true;
}