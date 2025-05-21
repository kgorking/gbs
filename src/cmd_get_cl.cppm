export module cmd_get_cl;
import std;
import compiler;
import context;
import env;

export bool cmd_get_cl(context& ctx, std::string_view args) {
	std::println("<gbs> get_cl : Searching for '{}'", args);

	if (std::optional<compiler> const opt_cl = get_compiler(ctx, args); opt_cl) {
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

	if (1 == std::system("curl --version >nul")) {
		std::println("<gbs> get_cl : 'curl' is required for this command to work");
		return false;
	}

	// Find the users folder
	auto homedir = get_home_dir();
	if (homedir.empty()) {
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
		cl.compiler = "bin/clang";

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
		cl.compiler = "bin/gcc";
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
	else if (cl.name == "msvc") {
		auto const vs_buildtools = homedir / ".gbs" / "vs_BuildTools.exe";
		std::println("<gbs>    downloading Visual Studio build tools...");
		if (0 != std::system(std::format("curl -fSL https://aka.ms/vs/17/release/vs_BuildTools.exe --output \"{}\"", vs_buildtools.generic_string()).c_str())) {
			std::filesystem::remove(vs_buildtools);
			std::println("<gbs>    Error downloading msvc build tools");
			return false;
		}

		std::string_view const vstools_args = "--passive --wait"
			" --add Microsoft.VisualStudio.Workload.VCTools"
			" --add Microsoft.VisualStudio.Component.VC.ASAN"
			" --add Microsoft.VisualStudio.Component.VC.Tools.x86.x64"
			" --add Microsoft.VisualStudio.Component.VC.Llvm.Clang"
			" --add Microsoft.VisualStudio.Component.VC.Modules.x86.x64"
			" --add Microsoft.VisualStudio.Component.VC.140"
			" --add Microsoft.VisualStudio.Component.VC.v141.x86.x64"
			" --add Microsoft.VisualStudio.ComponentGroup.VC.Tools.142.x86.x64";

		std::println("<gbs>    installing Visual Studio build tools, this will take a while...");
		if (0 != std::system(std::format("{} {}", vs_buildtools.generic_string(), vstools_args).c_str())) {
			std::filesystem::remove(vs_buildtools);
			std::println("<gbs>    Error installing msvc build tools");
			return false;
		}

		std::filesystem::remove(vs_buildtools);
		std::println("<gbs>    Download and install successful");
		return true;
	}
	else {
		std::println("<gbs>    Unsupported compiler {}", cl.name);
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
		bool found_valid = false;
		do {
			if (!(versions >> version))
				break;
			versions >> version;

			// Strip down to version
			version = version.substr(version_prefix.size());

			// Build url
			url = std::vformat(gh_download_url, std::make_format_args(version));

			// Check for file
			std::print("<gbs>    Checking for {} {} ...", cl.name, version);
			found_valid = !(22 == std::system(std::format("curl -fsI {} >nul", url).c_str()));
			std::puts(found_valid ? "Found" : "Not found");
		} while (!found_valid);

		if (!found_valid) {
			std::println("<gbs>    No suitable download found for {}:{}", cl.name, args);
			return false;
		}
	}
	std::remove("version_list.txt");

	extract_compiler_version(version, cl.major, cl.minor, cl.patch);

	std::string_view const filename = std::string_view{ url }.substr(1 + url.find_last_of('/'));
	auto const gbs_user_path = std::filesystem::path(homedir) / ".gbs";
	auto const downloaded_file_path = gbs_user_path / filename;
	auto const compiler_dir = gbs_user_path / cl.name;
	auto const download_dir = compiler_dir / std::vformat(extract_output_dir, std::make_format_args(version));
	auto const dest_dir = compiler_dir / std::format("{}_{}", cl.name, version);

	// Check if already downloaded
	cl.dir = dest_dir;
	cl.compiler = cl.dir / cl.compiler; // update exe path

	if (std::filesystem::exists(cl.dir)) {
		// TODO check bin

		std::println("<gbs>    Found {} {} in {}", cl.name, version, cl.dir.generic_string());
		return true;
	}

	// Prep the destination dir
	std::filesystem::create_directories(compiler_dir);

	// Download
	if (!std::filesystem::exists(downloaded_file_path)) {
		std::println("<gbs>    {}", url);
		if (0 != std::system(std::format("curl -fSL {} | tar -xf - -C {}", url, compiler_dir.generic_string()).c_str())) {
			std::println("<gbs> Error downloading and unpacking {} {}", cl.name, version);
			return false;
		}

		std::filesystem::rename(download_dir, dest_dir);
	}

	// Add it to the list of supported compilers
	ctx.all_compilers[cl.name].emplace_back(cl);
	ctx.selected_cl = cl;

	std::println("<gbs> Download successful");
	return true;
}