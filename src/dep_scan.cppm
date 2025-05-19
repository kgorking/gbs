export module dep_scan;
import std;

export struct source_dependency {
	std::filesystem::path path;
	std::string export_name;
	std::set<std::string> import_names;

	bool is_export() const {
		return !export_name.empty();
	}
};

// Returns a source files module dependencies.
export auto detect_module_dependencies(std::filesystem::path path) -> source_dependency {
	source_dependency dependencies{ path };

	std::string line;
	auto file = std::ifstream(path);
	while (std::getline(file, line)) {
		if (line.empty())
			continue;

		// TODO test for {}

		// Simple pattern: look for "\b(import|export module)\s+<module-name>;"
		std::string_view const sv = line;

		// Skip initial whitespaces
		auto const text_start = sv.find_first_not_of(" \t", 0);
		if (text_start == std::string_view::npos)
			continue;

		// Find the start of the module name
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

		// Skip whitespace
		auto const whitespaces = module_name.find_first_not_of(" \t");
		if (whitespaces != std::string_view::npos)
			module_name = module_name.substr(whitespaces);

		// Find end of module name (until ';' or whitespace)
		auto const end = module_name.find_first_of(" ;\t");
		if (end == std::string_view::npos)
			continue;

		// The full module name
		module_name = module_name.substr(0, end);

		if (is_export)
			dependencies.export_name.assign(module_name);
		else
			dependencies.import_names.emplace(module_name);
	}

	return dependencies;
	};
