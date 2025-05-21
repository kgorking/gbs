export module get_source_groups;
import std;
import dep_scan;
import monad;

namespace fs = std::filesystem;

// A set of imports for a module
using import_set = std::set<std::string>;

// Holds a single source files module dependencies,
// including its dependencies dependencies
export using source_info = std::pair<fs::path, import_set>;

// A single group of source files
// that can be compiled in parallel.
export using source_group = std::vector<source_info>;


// Find the source files and dependencies
export auto get_grouped_source_files(fs::path dir) -> std::map<std::size_t, source_group> {
	// Maps a filename to _all_ its module dependencies
	auto file_to_imports_map = std::unordered_map<fs::path, import_set>{};
	auto store_in_map = [&](source_dependency const& sd) { file_to_imports_map[sd.path.string()].insert_range(sd.import_names); };

	// Maps an export module name to its filename
	// and find all immediate dependencies for each file
	auto module_name_to_file_map = monad(fs::recursive_directory_iterator(dir))
			.filter(&fs::directory_entry::is_regular_file)
			.transform(&fs::directory_entry::path)
			.transform(detect_module_dependencies)
			.passthrough(store_in_map)
			.filter(&source_dependency::is_export)
			.as_tuple(&source_dependency::export_name, &source_dependency::path)
			.to<std::unordered_map>();


	// Recursively merge a files child dependencies into its current dependencies.
	//   Fx. A -> B -> C
	//   Results in A's dependencies being [B, C]
	auto const recursive_merge = [&](this auto& self, import_set& deps) -> void {
		for (auto const& dep : deps) {
			if (module_name_to_file_map.contains(dep)) {
				auto const& dep_path = module_name_to_file_map[dep];
				self(file_to_imports_map[dep_path]);
				deps.insert_range(file_to_imports_map[dep_path]);
			}
		}
		};
	std::ranges::for_each(file_to_imports_map | std::views::values, recursive_merge);

	// Group files according to how deep their dependency chain is
	auto sources = std::map<std::size_t, source_group>{};
	for (auto&& [path, imports] : file_to_imports_map) {
		auto const dep_size = imports.size();
		sources[dep_size].emplace_back(path, std::move(imports));
	}

	return sources;
}
