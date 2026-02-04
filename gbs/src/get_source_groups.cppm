module;
#include <filesystem>
#include <string>
#include <set>
#include <unordered_map>
#include <map>
#include <array>
export module get_source_groups;
import dep_scan;

namespace fs = std::filesystem;

// A set of imports for a module
export using import_set = std::set<std::string>;

// Holds a single source files module dependencies
export using source_info = std::pair<fs::path, import_set>;

// A map of source files to their module dependencies
using file_to_imports_map = std::unordered_map<fs::path, import_set>;

// A single group of source files that can be compiled in parallel
export using source_group = std::unordered_map<fs::path, import_set>;

// A map of source files grouped by their dependency depth
export using depth_ordered_sources_map = std::map<std::size_t, source_group>;


export bool should_not_exclude(fs::path const& path) {
	return
		!path.generic_string().starts_with("x.") &&
		!path.filename().stem().generic_string().starts_with("x.");
}

// Recursively merge a files child dependencies with its own dependencies.
//   Fx. A -> B -> C
//   Results in A's dependencies being [B, C]
static source_info recursive_merge(fs::path const&file, import_set const& deps, std::unordered_map<std::string, fs::path> const& module_name_to_file_map, file_to_imports_map const& file_imports) {
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

bool is_valid_sourcefile(fs::path const& file) {
	static constexpr std::array<std::string_view, 4> extensions{".cpp", ".c", ".cppm", ".ixx"};
	return extensions.end() != std::find(extensions.begin(), extensions.end(), file.extension());
}

// Find the source files and dependencies
export depth_ordered_sources_map get_grouped_source_files(fs::path const& dir) {
	file_to_imports_map file_imports;

	// Maps an export module name to its filename
	// and find all immediate dependencies for each file
	std::unordered_map<std::string, fs::path> module_name_to_file_map;
	for (auto const& dir_it : fs::recursive_directory_iterator(dir)) {
		if (!dir_it.is_regular_file())
			continue;

		fs::path const file_path = dir_it.path();
		if (!is_valid_sourcefile(file_path) || !should_not_exclude(file_path))
			continue;

		source_dependency const sd = extract_module_dependencies(file_path);
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
