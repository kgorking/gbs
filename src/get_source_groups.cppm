module;
export module get_source_groups;
import std;
import dep_scan;
import monad;

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


// Maps a filename to _all_ its module dependencies
void store_in_map(source_dependency const& sd, file_to_imports_map& fi) {
	fi[sd.path] = sd.import_names;
}

// Recursively merge a files child dependencies with its own dependencies.
//   Fx. A -> B -> C
//   Results in A's dependencies being [B, C]
source_info recursive_merge(fs::path const&file, import_set const& deps, std::unordered_map<std::string, fs::path> const& module_name_to_file_map, file_to_imports_map const& file_imports) {
	//auto const& [file, deps] = pair;
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
	auto const& [path, imports] = si;
	std::size_t const dep_size = imports.size();
	sources[dep_size][path].merge(auto(imports));
}

bool is_valid_sourcefile(fs::path const& file) {
	static constexpr std::array<std::string_view, 4> extensions{".cpp", ".c", ".cppm", ".ixx"};
	return std::ranges::contains(extensions, file.extension());
}

// Find the source files and dependencies
export depth_ordered_sources_map get_grouped_source_files(fs::path const& dir) {
	file_to_imports_map file_imports;

	// Maps an export module name to its filename
	// and find all immediate dependencies for each file
	auto const module_name_to_file_map = as_monad(fs::recursive_directory_iterator(dir))
		.join()
		.filter(&fs::directory_entry::is_regular_file)
		.map(&fs::directory_entry::path)
		.filter(is_valid_sourcefile)
		.map(extract_module_dependencies)
		.and_then(store_in_map, std::ref(file_imports))
		.filter(&source_dependency::is_export)
		.to<std::unordered_map>(&source_dependency::export_name, &source_dependency::path);

	// Merge all dependencies for each file and
	// return an ordered map of files grouped by their dependency depth
	return as_monad(file_imports)
		.join()
		.map(recursive_merge, module_name_to_file_map, file_imports)
		.to_dest(group_by_dependency_depth);
}
