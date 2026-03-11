#include "../inc/context.h"
#include "../inc/dep_scan.h"
#include <format>
#include <fstream>
#include <print>

// Returns a source files module dependencies.
auto extract_module_dependencies(context const& ctx, std::filesystem::path path) -> source_dependency {
	source_dependency dependencies{ path };

	auto const& cl = ctx.get_selected_compiler();
	if (cl.wsl && !std::filesystem::exists(path)) {
		std::string new_path = std::format(R"(\\wsl.localhost\{}{})", *cl.wsl, path.string());
		if (std::filesystem::exists(new_path)) {
			path = new_path;
		}
		else {
			std::println("<gbs-depscan> error: file '{}' doesn't exist for compiler {}, aborting dependency scan", path.string(), cl.name_and_version);
			return dependencies;
		}
	}

	bool const is_msvc = ctx.compiler_name() == "msvc";
	std::string main_module = "";
	std::string line;
	auto file = std::ifstream(path);
	if (!file) {
		std::println("<gbs-depscan> error: failed to open file '{}'", path.generic_string());
		return dependencies;
	}

	while (std::getline(file, line)) {
		if (line.empty())
			continue;

		// TODO test for {}

		// Simple pattern: look for "\b(import|export module)\s+<module-name>;"
		//std::string_view const sv = line;

		// Skip initial whitespaces
		auto const text_start = line.find_first_not_of(" \t", 0);
		if (text_start == std::string_view::npos)
			continue;

		// Find the start of the module name
		bool is_export = false;
		auto module_name = line.substr(text_start);
		if (module_name.starts_with("import ")) {
			module_name = module_name.substr(7);
		}
		else if (module_name.starts_with("export module ")) {
			is_export = true;
			module_name = module_name.substr(14);
		}
		else if (module_name.starts_with("export import ")) {
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
		if (module_name.empty())
			continue;

		if (module_name[0] == ':') {
			module_name = main_module + module_name.substr(1);
		}
		else if (module_name.contains(':')) {
			auto const global_module_pos = module_name.find(':');
			if (is_msvc) {
				main_module = module_name.substr(0, global_module_pos) + '-';
				module_name[module_name.find(':')] = '-';
			}
			else {
				module_name = module_name.substr(global_module_pos + 1);
			}
		}
		else if (is_export) {
			if (is_msvc)
				main_module = module_name + '-';
		}

		if (is_export)
			dependencies.export_name.assign(module_name);
		else
			dependencies.import_names.emplace(module_name);
	}

	return dependencies;
};
