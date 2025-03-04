// Enumerate all sources in project tree
// 
// Output findings to appropriate response files
//   - modules, sources, objects
#include <filesystem>
#include <fstream>
#include "compiler.h"
#include "context.h"

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
		}

		auto const out = (output_dir / it.path().filename()).replace_extension("obj");
		if (is_file_up_to_date(it.path(), out)) {
			ctx.objects << ' ' << out;
		}
		else {
			ctx.sources << ' ' << it.path().generic_string();
		}
	}
}

void enumerate_sources(context const& ctx, std::filesystem::path dir, std::filesystem::path output_dir) {
	enum_context e_ctx{
		std::ofstream(ctx.response_dir() / "modules", std::ios::out | std::ios::trunc),
		std::ofstream(ctx.response_dir() / "sources", std::ios::out | std::ios::trunc),
		std::ofstream(ctx.response_dir() / "objects", std::ios::out | std::ios::trunc)
	};

	enumerate_sources_imp(e_ctx, dir, output_dir);
}
