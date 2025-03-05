#include <string_view>
#include <fstream>
#include <print>
#include "context.h"
#include "home.h"

bool get_cl(context& ctx, std::string_view args) {
	std::println("<gbs> Searching for {}", args);

	if (std::optional<compiler> opt_cl = get_compiler(ctx, args); opt_cl) {
		ctx.selected_cl = *opt_cl;
		return true;
	}

	if (args.empty()) {
		std::println("<gbs> error : usage 'get_cl=compilername:version', eg. 'get_cl=clang:19.1'");
		return false;
	}

	if (1 == std::system("git --version >nul")) {
		std::println("<gbs> 'git' is required for this command to work");
		return false;
	}


	if (1 == std::system("tar --version >nul")) {
		std::println("<gbs> 'tar' is required for this command to work");
		return false;
	}

	// Find the users folder
	char const* homedir = get_home_dir();
	if (!homedir) {
		std::println("<gbs> Unable to get home directory");
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
	if (args.empty())
		args = ":";

	if (args.contains(':')) {
		args.remove_prefix(1);
		if (args.empty())
			args = "[0-9]";
			
		// Version to search for
		version = args;
	}


	if (cl.name == "clang") {
		cl.exe = "bin/clang";

		version_prefix = "refs/tags/llvmorg-";

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
		// git ls-remote --exit-code --refs --tags --sort="-version:refname" https://github.com/xpack-dev-tools/gcc-xpack v*
		// https://github.com/xpack-dev-tools/gcc-xpack/releases/download/v14.2.0-2/xpack-gcc-14.2.0-2-win32-x64.zip
		cl.exe = "bin/gcc";
		version_prefix = "refs/tags/v";

		git_search_cmd = std::format("git ls-remote --exit-code --refs --tags --sort=\"-version:refname\" https://github.com/xpack-dev-tools/gcc-xpack *v{}* > {}",
			version,
			"version_list.txt");

#ifdef _WIN64
		cl.arch = "x64";
		gh_download_url = "https://github.com/xpack-dev-tools/gcc-xpack/releases/download/v{0}/xpack-gcc-{0}-win32-x64.zip";
		extract_output_dir = "xpack-gcc-{0}";
#elif __linux__
		?? cl.arch = "x64";
		gh_download_url = "https://github.com/llvm/llvm-project/releases/download/llvmorg-{0}/clang+llvm-{0}-aarch64-linux-gnu.tar.xz";
		extract_output_dir = "clang+llvm-{0}-aarch64-linux-gnu";
#elif _ARM // ??
		?? cl.arch = "arm64";
		gh_download_url = "https://github.com/llvm/llvm-project/releases/download/llvmorg-{0}/clang+llvm-{0}-armv7a-linux-gnueabihf.tar.gz";
		extract_output_dir = "clang+llvm-{0}-armv7a-linux-gnueabihf";
#endif
	}
	else {
		std::println("<gbs> Only clang and gcc downloads supported so far");
		return false;
	}

	// Search for compiler version
	if (2 == std::system(git_search_cmd.c_str())) {
		std::remove("version_list.txt");
		std::println("<gbs>    no valid version found");
		return false;
	}

	// Load the closest version from the version file
	std::string url;
	{
		std::ifstream versions("version_list.txt");
		do {
			versions >> version;
			versions >> version;

			// Strip down to version
			version = version.substr(version_prefix.size());

			// Build url
			url = std::vformat(gh_download_url, std::make_format_args(version));
		} while (22 == std::system(std::format("curl -f -s -I {} >nul", url).c_str()));
	}
	std::remove("version_list.txt");

	extract_version(version, cl.major, cl.minor);

	// Check if already downloaded
	std::string const dirname = std::vformat(extract_output_dir, std::make_format_args(version));
	cl.dir = std::filesystem::path(homedir) / ".gbs" / dirname;
	cl.exe = cl.dir / cl.exe; // update exe path

	if (std::filesystem::exists(cl.dir)) {
		// TODO check bin

		std::println("<gbs>    Found {} {} in {}", cl.name, version, cl.dir.generic_string());
		return true;
	}

	std::string_view const filename = std::string_view{ url }.substr(1 + url.find_last_of('/'));
	std::string const gbs_user_path = std::format("{}/.gbs/", homedir);
	std::string const downloaded_file_path = std::format("{}/.gbs/{}", homedir, filename);

	// Download
	if (!std::filesystem::exists(downloaded_file_path)) {
		std::println("<gbs>    downloading and unpacking {} {}", cl.name, version);
		std::println("<gbs>    {}", url);
		if (0 != std::system(std::format("curl -fSL {} | tar -xf - -C {}", url, gbs_user_path).c_str())) {
			std::println("<gbs> Error downloading and unpacking {} {}", cl.name, version);
			return false;
		}
	}

	// Add it to the list of supported compilers
	ctx.all_compilers[cl.name].emplace_back(cl);
	ctx.selected_cl = cl;

	std::println("<gbs> Download successful");
	return true;
}