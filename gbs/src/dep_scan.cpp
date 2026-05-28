#include "../inc/context.h"
#include "../inc/dep_scan.h"
#include <format>
#include <fstream>
#include <print>

// Returns a source files module dependencies.
auto extract_module_dependencies(context const&, std::filesystem::path path) -> source_dependency {
	source_dependency dependencies{ .path = path };

	std::string main_module_name;
	auto file = std::ifstream(path);
	if (!file) {
		std::println("<gbs-depscan> error: failed to open file '{}'", path.generic_string());
		return dependencies;
	}

	std::string line;
	while (std::getline(file, line)) {
		if (line.empty())
			continue;

		// TODO test for {}

		// Simple pattern: look for "\b(import|export module)\s+<module-name>;"
		//std::string_view const sv = line;

		// Skip initial whitespaces
		std::string_view module_line = line;
		std::size_t whitespaces = module_line.find_first_not_of(" \t");
		if (whitespaces == std::string_view::npos) // whole string is whitespace
			continue;

		module_line = module_line.substr(whitespaces);
		if (module_line.empty())
			continue;

		// Find the start of the module name
		bool is_export = false;
		if (module_line.starts_with("import ")) {
			// Import
			is_export = false;
			module_line = module_line.substr(7);
		}
		else if (module_line.starts_with("export module ")) {
			// Create module
			is_export = true;
			module_line = module_line.substr(14);
		}
		else if (module_line.starts_with("export import ")) {
			// Re-export module
			module_line = module_line.substr(14);
		}
		else {
			continue;
		}

		// Skip whitespace
		whitespaces = module_line.find_first_not_of(" \t");
		if (whitespaces != std::string_view::npos)
			module_line = module_line.substr(whitespaces);

		// Find end of module name (until ';' or whitespace)
		auto const end = module_line.find_first_of(" ;\t");
		if (end == std::string_view::npos)
			continue;

		// The full module name
		module_line = module_line.substr(0, end);
		if (module_line.empty())
			continue;

		module_desc module{};
		if (module_line[0] == ':') { // module :part;
			module.name = main_module_name;
			module.partition = module_line.substr(1);
		}
		else if (module_line.contains(':')) { // module name:part;
			auto const global_module_pos = module_line.find(':');
			module.name = module_line.substr(0, global_module_pos);
			module.partition = module_line.substr(global_module_pos + 1);
			/*if (is_msvc) {
				main_module = module_line.substr(0, global_module_pos) + '-';
				module_line[module_line.find(':')] = '-';
			}
			else {
				module_line = module_line.substr(global_module_pos + 1);
			}*/
		}
		else {
			module.name = module_line;
		}

		if (is_export) {
			main_module_name = module.name;
			dependencies.export_name = module;
		}
		else
			dependencies.import_names.emplace(module);
	}

	return dependencies;
};
