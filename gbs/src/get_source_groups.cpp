#include "../inc/context.h"
#include "../inc/dep_scan.h"
#include "../inc/get_source_groups.h"
#include "../inc/os.h"
#include <array>
#include <generator>
#include <set>

namespace fs = std::filesystem;

bool should_include(context const& ctx, fs::path const& path) {
	auto const& generic_path = path.generic_string();

	// Check if file or directory name starts with "x."
	if (generic_path.starts_with("x.") || 
		path.filename().stem().generic_string().starts_with("x."))
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
static source_info recursive_merge(fs::path const&file, import_set const& deps, std::map<std::string, fs::path> const& module_name_to_file_map, file_to_imports_map const& file_imports) {
	import_set all_merged_deps{ deps };

	for (auto const& dep : deps) {
		if (module_name_to_file_map.contains(dep)) {
			auto const& dep_path = module_name_to_file_map.at(dep);
			auto [fst, snd] = recursive_merge(dep_path, file_imports.at(dep_path), module_name_to_file_map, file_imports);
			all_merged_deps.insert_range(std::move(snd));
		}
	}

	return { file, all_merged_deps };
}

// Group files according to how deep their dependency chain is
void group_by_dependency_depth(depth_ordered_sources_map& sources, source_info const& si) {
	auto [path, imports] = si;
	std::size_t const dep_size = imports.size();
	sources[dep_size][path].merge(imports);
}

static bool is_valid_sourcefile(fs::path const& file) {
	static constexpr std::array<std::string_view, 4> extensions{".cpp", ".c", ".cppm", ".ixx"};
	return extensions.end() != std::find(extensions.begin(), extensions.end(), file.extension());
}

std::generator<fs::path> get_source_files(context const& ctx, fs::path const& dir) {
	for (auto const& dir_it : fs::recursive_directory_iterator(dir)) {
		if (!dir_it.is_regular_file())
			continue;
		fs::path const file_path = dir_it.path();
		if (!is_valid_sourcefile(file_path) || !should_include(ctx, file_path))
			continue;
		co_yield file_path;
	}
}

depth_ordered_sources_map get_grouped_source_files(context const& ctx, fs::path const& dir) {
	file_to_imports_map file_imports;

	// Maps an export module name to its filename
	// and find all immediate dependencies for each file
	std::map<std::string, fs::path> module_name_to_file_map;
	for (auto const& dir_it : fs::recursive_directory_iterator(dir)) {
		if (!dir_it.is_regular_file())
			continue;

		fs::path const file_path = dir_it.path();
		if (!is_valid_sourcefile(file_path) || !should_include(ctx, file_path))
			continue;

		source_dependency const sd = extract_module_dependencies(ctx, file_path);
		file_imports[sd.path] = sd.import_names;

		module_name_to_file_map.insert({ sd.export_name, sd.path });
	}

	// Merge all dependencies for each file and
	// return an ordered map of files grouped by their dependency depth
	depth_ordered_sources_map sources;
	for (auto const& [file, imports] : file_imports) {
		source_info const merged = recursive_merge(file, imports, module_name_to_file_map, file_imports);
		group_by_dependency_depth(sources, merged);
	}

	return sources;
}
