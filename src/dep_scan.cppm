module;
#include <filesystem>
#include <string>
#include <string_view>
#include <fstream>
#include <unordered_map>
export module dep_scan;
//import std;

struct dep_entry {
	std::filesystem::path file;
	std::string export_name;
	std::vector<std::string> import_names;
};

// Returns a vector of module dependencies for each module file found.
// First entry in the vector, is the exported modules name.
export auto detect_module_dependencies(std::filesystem::path dir) -> std::vector<dep_entry> {
	std::vector<dep_entry> dependencies;

	for (auto const& entry : std::filesystem::recursive_directory_iterator(dir)) {
		if (!entry.is_regular_file())
			continue;

		// Only scan modules
		auto path = entry.path();
		if (path.extension() != ".cppm")
			continue;

		std::ifstream file(path);
		if (!file.is_open())
			continue;

		auto& module_deps = dependencies.emplace_back(path);

		std::string line;
		while (std::getline(file, line)) {
			if (line.empty())
				continue;

			// Simple pattern: look for "\b(import|export module)\s+<module-name>;"
			std::string_view sv = line;

			// Skip initial whitespaces
			auto text_start = sv.find_first_not_of(" \t", 0);
			if (text_start == std::string_view::npos)
				continue;

			bool is_export = false;
			auto module_name = sv.substr(text_start);
			if (module_name.starts_with("import ")) {
				module_name = module_name.substr(7);
			}
			else if (module_name.starts_with("export module ")) {
				is_export = true;
				module_name = module_name.substr(14);
			}
			else {
				continue;
			}

			// Find the start of the module name

			// Skip whitespace
			auto const whitespaces = module_name.find_first_not_of(" \t");
			if (whitespaces != std::string_view::npos)
				module_name = module_name.substr(whitespaces);

			// Find end of module name (until ';' or whitespace)
			auto end = module_name.find_first_of(" ;\t");
			if (end == std::string_view::npos)
				continue;

			// Ignore 'std' imports
			module_name = module_name.substr(0, end);
			if (module_name == "std")
				continue;

			if (is_export)
				module_deps.export_name = { module_name.data(), module_name.size() };
			else
				module_deps.import_names.emplace_back(module_name.data(), module_name.size());
		}
	}

	return dependencies;
}
