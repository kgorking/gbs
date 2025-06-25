export module get_source_groups;
import std;
import dep_scan;
import monad;

namespace fs = std::filesystem;

using import_set = std::unordered_set<std::string>;
using imports_for_file_map = std::unordered_map<fs::path, import_set>;
using file_imports_pair = typename imports_for_file_map::value_type;

// Holds a single source files module dependencies,
// including its dependencies dependencies
export using source_info = std::pair<fs::path, import_set>;

// A single group of source files
// that can be compiled in parallel.
export using source_group = std::vector<source_info>;

// A map of source groups, indexed by the number of dependencies.
export using sorted_group_map = std::map<std::size_t, source_group>;


// Find the source files and dependencies
export auto get_grouped_source_files(fs::path const& dir) -> sorted_group_map {
	// Maps a filename to _all_ its module dependencies
	auto file_imports = imports_for_file_map{};

	auto store_in_map = [&](source_dependency const& sd) {
		file_imports[sd.path.string()].insert_range(sd.import_names);
		};

	// Maps an export module name to its filename
	// and find all immediate dependencies for each file
	using module_name_to_file_map = std::unordered_map<std::string, fs::path>;
	auto const mod_to_files = monad(fs::recursive_directory_iterator(dir))
			.filter(&fs::directory_entry::is_regular_file)
			.map(detect_module_dependencies)
			.and_then(store_in_map)
			.filter(&source_dependency::is_export)
			.as_tuple(&source_dependency::export_name, &source_dependency::path)
			.to<module_name_to_file_map>();

	// Recursively merge a files child dependencies into its current dependencies.
	//   Fx. A -> B -> C
	//   Results in A's dependencies being [B, C]
	auto recursive_merge = [&](this auto& self, import_set imports) -> import_set {
		return monad(imports)
			.filter([&](std::string const& mod) { return mod_to_files.contains(mod); })
			.map   ([&](std::string const& mod) { return self(file_imports[mod_to_files.at(mod)]); })
			.to_dest(imports);
		};

	auto to_source_info = [&](file_imports_pair const& pair) {
		return source_info{ pair.first, recursive_merge(pair.second) };
		};
	auto insert_into_sources_map = [](sorted_group_map& map, source_info const& si) {
		map[si.second.size()].emplace_back(si.first, si.second);
		};

	// Group files according to how deep their dependency chain is
	return monad(file_imports)
		.map(to_source_info)
		.to_dest<sorted_group_map>(insert_into_sources_map);
}
