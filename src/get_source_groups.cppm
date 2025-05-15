export module get_source_groups;
import std;
import dep_scan;

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
export auto get_grouped_source_files(fs::path dir) -> std::vector<source_group> {
	// Maps a filename to _all_ its module dependencies
	auto file_to_imports_map = std::unordered_map<fs::path, import_set>{};

	// Maps an export module name to its filename
	auto module_name_to_file_map = std::unordered_map<std::string, fs::path>{};


	// Find all immediate dependencies for each file
	for (fs::directory_entry it : fs::recursive_directory_iterator(dir)) {
		if (it.is_directory())
			continue;

		auto const path = it.path();

		auto const deps = detect_module_dependencies(path);
		file_to_imports_map[path.string()].insert_range(deps.import_names);

		if (!deps.export_name.empty())
			module_name_to_file_map[deps.export_name] = path.string();
	}

	// Recursively merge a files child dependencies into its current dependencies.
	//   Fx. A -> B -> C
	//   Results in A's dependencies being [B, C]
	std::ranges::for_each(file_to_imports_map | std::views::values, [&](this auto& self, import_set& deps) -> void {
		for (auto const& dep : deps) {
			if (module_name_to_file_map.contains(dep)) {
				auto const& dep_path = module_name_to_file_map[dep];
				self(file_to_imports_map[dep_path]);
				deps.insert_range(file_to_imports_map[dep_path]);
			}
		}
		});

	// Group files according to how deep their dependency chain is
	auto sources = std::vector<source_group>{};
	for (auto&& [path, imports] : file_to_imports_map) {
		auto const dep_size = imports.size();

		while (sources.size() <= dep_size) {
			sources.emplace_back();
		}

		sources.at(dep_size).emplace_back(path, std::move(imports));
	}

	return sources;
}
