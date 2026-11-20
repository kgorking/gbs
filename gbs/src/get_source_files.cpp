#include "../inc/context.h"
#include "../inc/dep_scan.h"
#include "../inc/get_source_files.h"
#include "../inc/os.h"
#include <array>
#include <filesystem>
#include <generator>
#include <set>

namespace fs = std::filesystem;

bool should_include(context const& ctx, fs::path const& path) {
	auto const& generic_path = path.generic_string();

	// Check if file or directory name starts with "x."
	if (generic_path.starts_with("x.") || path.filename().stem().generic_string().starts_with("x."))
		return false;

	// Check OS-specific directory constraints
	auto const target_os = ctx.get_target_os();

	for (auto const& component : path) {
		auto const component_str = component.generic_string();

		if (component_str == "windows" && target_os != operating_system::windows)
			return false;

		if (component_str == "linux" && target_os != operating_system::linux)
			return false;

		if (component_str == "macos" && target_os != operating_system::macos)
			return false;
	}
	return true;
}

static bool is_valid_sourcefile(fs::path const& file) {
	static constexpr std::array<std::string_view, 4> extensions{".cpp", ".c", ".cppm", ".ixx"};
	return extensions.end() != std::find(extensions.begin(), extensions.end(), file.extension());
}

// Adjust the path for WSL if necessary
static fs::path adjust_for_wsl(compiler const& cl, fs::path source_path) {
	if (cl.wsl && !std::filesystem::exists(source_path)) {
		std::string new_path = std::format(R"(\\wsl.localhost\{}{})", *cl.wsl, source_path.string());
		if (std::filesystem::exists(new_path)) {
			return new_path;
		}
	}

	return source_path;
}

std::generator<fs::path> get_source_files(context const& ctx, compiler const& cl, fs::path dir) {
	for (auto const& dir_it : fs::recursive_directory_iterator(dir, fs::directory_options::follow_directory_symlink)) {
		if (!dir_it.is_regular_file())
			continue;

		fs::path file_path = adjust_for_wsl(cl, dir_it.path());
		if (is_valid_sourcefile(file_path) && should_include(ctx, file_path)) {
			co_yield file_path;
		}
	}
}
