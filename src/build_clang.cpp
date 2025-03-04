#include <string_view>
#include <filesystem>
#include <format>
#include <print>
#include <ranges>

#include "context.h"
#include "response.h"

namespace fs = std::filesystem;

bool build_clang(context& ctx, std::string_view args) {
	std::string cmd;

	std::string const resp_dir = (ctx.response_dir()).generic_string();

	// Convert ',' into response paths
	// eg. 'a,b,c' -> 'a @{path}/b @{path}/c'
	auto view_args = args | std::views::split(',');
	auto view_resp = view_args | std::views::join_with(std::format(" @{}/", resp_dir));

	// Build response file argument
	std::string const resp_args = std::format("@{0}/_shared @{0}/{1:s}", resp_dir, view_resp);

	// Build modules/files/objects argument
	std::string const files_args = std::format(" @{0}/modules @{0}/sources @{0}/objects", resp_dir);

	// Build output
	ctx.output_config = std::string_view{ view_args.front() };
	auto const output_dir = ctx.output_dir();

	// Create the build dirs if needed
	std::filesystem::create_directories(output_dir);

	// Add source files
	std::string const executable = fs::current_path().stem().string() + ".exe";
	cmd += std::format("\"{}\" {} {} -o={}", ctx.selected_cl.exe.generic_string(), resp_args, files_args, executable);
	// && mv f.o gbs.out/clang/debug/

	//extern void enumerate_sources(context const&, std::filesystem::path, std::filesystem::path);
	//enumerate_sources(ctx, "src", output_dir);

	using namespace std::filesystem;

	//for (directory_entry it : directory_iterator("src")) {
	//	if (it.is_directory()) {
	//		enumerate_sources_imp(ctx, it.path(), output_dir);
	//	}
	//}
	//
	//
	//for (directory_entry it : directory_iterator(dir)) {
	//	if (it.is_directory())
	//		continue;
	//
	//	if (it.path().extension() == ".ixx") {
	//		auto const ifc = (output_dir / it.path().filename()).replace_extension("ifc");
	//		//ctx.modules << std::format(" -fmodule-file={}={}", ifc.stem(), ifc.generic_string());
	//	}
	//
	//	auto const out = (output_dir / it.path().filename()).replace_extension("obj");
	//	if (is_file_up_to_date(it.path(), out)) {
	//		ctx.objects << out << ' ';
	//	}
	//	else {
	//		ctx.sources << "-c " << it.path().generic_string() << ' ';// << " -o " << it.path().stem() << ".o ";
	//	}
	//}


	return 0 == std::system(cmd.c_str());
}
