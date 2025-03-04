#include <string_view>
#include <filesystem>
#include <format>
#include <print>
#include <ranges>
#include <fstream>

#include "context.h"
#include "response.h"

namespace fs = std::filesystem;

struct enum_context {
	std::ofstream modules;
	std::ofstream sources;
	std::ofstream objects;
};

void enumerate_sources_imp(enum_context& ctx, std::filesystem::path dir, std::filesystem::path output_dir) {
	using namespace std::filesystem;

	for (directory_entry it : directory_iterator(dir)) {
		if (it.is_directory()) {
			enumerate_sources_imp(ctx, it.path(), output_dir);
		}
	}

	for (directory_entry it : directory_iterator(dir)) {
		if (it.is_directory())
			continue;

		if (it.path().extension() == ".cpp") {
			auto const ifc = (output_dir / it.path().filename()).replace_extension("ifc");
			ctx.modules << std::format(" /reference {}", ifc.generic_string());
		}

		auto const out = (output_dir / it.path().filename()).replace_extension("obj");
		if (is_file_up_to_date(it.path(), out)) {
			ctx.objects << out << ' ';
		}
		else {
			ctx.sources << it.path().generic_string() << ' ';
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

bool build_msvc(context& ctx, std::string_view args) {
	// Set up the build environment
	auto const vcvars = ctx.selected_cl.dir / "../../../Auxiliary/Build/vcvars64.bat";
	std::string cmd = std::format("\"\"{}\" ", vcvars.generic_string());

	// Arguments to the compiler(s)
	// Converts arguments into response files
	auto view_args = args | std::views::split(',');
	auto view_resp = view_args | std::views::join_with(std::format(" @{}/", (ctx.gbs_internal / ctx.selected_cl.name).generic_string()));

	// Build output
	ctx.output_config = std::string_view{ view_args.front() };
	auto const output_dir = ctx.output_dir();

	// Create the build dirs if needed
	std::filesystem::create_directories(output_dir);

	// Compile stdlib if needed
	if (!fs::exists(output_dir / "std.obj")) {
		fs::path const stdlib_module = ctx.selected_cl.dir / "modules/std.ixx";
		if (!is_file_up_to_date(stdlib_module, output_dir / "std.obj"))
			cmd += std::format(" && cl @{0}/_shared @{0}/{1:s} /c \"{2}\"", ctx.response_dir().generic_string(), view_resp, stdlib_module.generic_string());
	}

	// Add source files
	std::string const executable = fs::current_path().stem().string() + ".exe";
	cmd += std::format(" && cl @{0}/_shared @{0}/{1:s} /reference std={2}/std.ifc /Fe:{2}/{3}", ctx.response_dir().generic_string(), view_resp, output_dir.generic_string(), executable);
	auto not_dir = [](fs::directory_entry const& dir) { return !dir.is_directory(); };

	extern void enumerate_sources(context const&, std::filesystem::path, std::filesystem::path);
	enumerate_sources(ctx, "src", output_dir);

	cmd += std::format(" @{0}/modules @{0}/sources @{0}/objects\"", ctx.response_dir().generic_string());

	return 0 == std::system(cmd.c_str());
}
