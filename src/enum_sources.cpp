// Enumerate all sources in project tree
// 
// Output findings to appropriate response files
//   - modules, sources, objects
#include <filesystem>
#include <fstream>
#include "compiler.h"

using namespace std::filesystem;
using namespace std::string_view_literals;

struct enum_context {
	std::ofstream modules;
	std::ofstream sources;
	std::ofstream objects;
};

void enumerate_sources_imp(enum_context& ctx, std::filesystem::path dir, std::filesystem::path output_dir) {
	for (directory_entry it : directory_iterator(dir)) {
		if (it.is_directory()) {
			enumerate_sources_imp(ctx, it.path(), output_dir);
		}
	}


	for (directory_entry it : directory_iterator(dir)) {
		if (it.is_directory())
			continue;

		if (it.path().extension() == ".ixx") {
			auto const ifc = (output_dir / it.path().filename()).replace_extension("ifc");
			ctx.modules << std::format(" /reference {}", ifc.generic_string());

			auto const out = (output_dir / it.path().filename()).replace_extension("obj");
			if (is_file_up_to_date(it.path(), out)) {
				ctx.objects << ' ' << out;
			}
			else {
				ctx.modules << std::format(" {}", it.path().generic_string());
			}
		}
		else if (it.path().extension() == ".cpp") {
			auto const out = (output_dir / it.path().filename()).replace_extension("obj");
			if (is_file_up_to_date(it.path(), out)) {
				ctx.objects << ' ' << out;
			}
			else {
				ctx.sources << ' ' << it.path().generic_string();
			}
		}
	}
}

void enumerate_sources(std::filesystem::path dir, std::filesystem::path output_dir) {
	enum_context ctx{
		std::ofstream("out/modules", std::ios::out | std::ios::trunc),
		std::ofstream("out/sources", std::ios::out | std::ios::trunc),
		std::ofstream("out/objects", std::ios::out | std::ios::trunc)
	};

	enumerate_sources_imp(ctx, dir, output_dir);
}
