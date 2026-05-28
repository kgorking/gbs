#include "../inc/context.h"
#include "../inc/dep_scan.h"
#include "../inc/get_source_groups.h"
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

// Recursively merge a files child dependencies with its own dependencies.
//   Fx. A -> B -> C
//   Results in A's dependencies being [B, C]
/*static source_info recursive_merge(fs::path const& file, import_set const& deps, std::map<std::string, fs::path> const& module_name_to_file_map, file_to_imports_map const& file_imports) {
	import_set all_merged_deps{ deps };

	for (auto const& dep : deps) {
		if (module_name_to_file_map.contains(dep)) {
			auto const& dep_path = module_name_to_file_map.at(dep);
			auto [fst, snd] = recursive_merge(dep_path, file_imports.at(dep_path), module_name_to_file_map, file_imports);
			all_merged_deps.insert_range(std::move(snd));
		}
	}

	return { file, all_merged_deps };
}*/

static bool is_valid_sourcefile(fs::path const& file) {
	static constexpr std::array<std::string_view, 4> extensions{".cpp", ".c", ".cppm", ".ixx"};
	return extensions.end() != std::find(extensions.begin(), extensions.end(), file.extension());
}

std::generator<fs::path> get_source_files(context const& ctx, fs::path const& dir) {
	compiler const& cl = ctx.get_selected_compiler();

	for (auto const& dir_it : fs::recursive_directory_iterator(dir)) {
		if (!dir_it.is_regular_file())
			continue;

		fs::path file_path = dir_it.path();
		if (!is_valid_sourcefile(file_path) || !should_include(ctx, file_path))
			continue;

		if (cl.wsl && !std::filesystem::exists(file_path)) {
			std::string new_path = std::format(R"(\\wsl.localhost\{}{})", *cl.wsl, file_path.string());
			if (std::filesystem::exists(new_path)) {
				file_path = new_path;
			}
		}

		co_yield file_path;
	}
}
