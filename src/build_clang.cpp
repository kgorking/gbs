#include <string_view>
#include <filesystem>
#include <format>
#include <print>
#include <ranges>

#include "context.h"
#include "response.h"

namespace fs = std::filesystem;

bool build_clang(context& ctx, std::string_view args) {
	std::string const resp_dir = (ctx.response_dir()).generic_string();

	// Convert ',' into response paths
	// eg. 'a,b,c' -> 'a @{path}/b @{path}/c'
	auto view_args = args | std::views::split(',');
	auto view_resp = view_args | std::views::join_with(std::format(" @{}/", resp_dir));

	// Build response file argument
	std::string const resp_args = std::format("@{0}/_shared @{0}/{1:s}", resp_dir, view_resp);

	// Build output
	ctx.output_config = std::string_view{ view_args.front() };
	auto const output_dir = ctx.output_dir();

	// Create the build dirs if needed
	std::filesystem::create_directories(output_dir);

	// Compile std library
	// https://raw.githubusercontent.com/llvm/llvm-project/refs/heads/main/libcxx/modules/std.cppm.in

	// Compile modules
	using namespace std::filesystem;

	std::vector<std::filesystem::path> modules;
	std::vector<std::string> objects;

	auto compile_cppm = [&](std::filesystem::path const& in) -> void {
		auto const pcm = (output_dir / in.filename()).replace_extension("pcm");
		auto const obj = (output_dir / in.filename()).replace_extension("o");
		auto const cmd = std::format("call \"{}\" {} {} -fprebuilt-module-path=\"{}\" -c -fmodule-output={} -o {}", ctx.selected_cl.exe.string(), resp_args, in.generic_string(), output_dir.generic_string(), pcm.generic_string(), obj.generic_string());
		//std::puts(cmd.c_str());
		if (0 == std::system(cmd.c_str())) {
			std::puts(in.generic_string().c_str());
			objects.push_back(obj.generic_string());
		}
		else
			modules.push_back(in);
		};

	auto compile_modules = [&](this auto& self, std::filesystem::path p) -> bool {
		for (directory_entry it : directory_iterator(p)) {
			if (it.is_directory()) {
				self(it.path());
			}
		}

		for (directory_entry it : directory_iterator(p)) {
			if (it.is_directory())
				continue;

			if (it.path().extension() == ".cppm") {
				compile_cppm(it.path());
			}
		}

		while (!modules.empty()) {
			auto copy = modules;
			modules.clear();

			for(auto const& mod : copy)
				compile_cppm(mod);

			if (copy.size() == modules.size()) {
				std::println("<gbs> Could not resolve stuff?");
				return false;
			}
		}

		return true;
		};

	if (!compile_modules("src"))
		return false;

	// Compile sources
	auto compile_sources = [&](this auto& self, std::filesystem::path p) -> bool {
		for (directory_entry it : directory_iterator(p)) {
			if (it.is_directory()) {
				self(it.path());
			}
		}

		for (directory_entry it : directory_iterator(p)) {
			if (it.is_directory())
				continue;

			auto const& in = it.path();
			if (it.path().extension() == ".cpp") {
				auto const obj = (output_dir / in.filename()).replace_extension("o");
				auto const cmd = std::format("call \"{}\" {} {} -fprebuilt-module-path=\"{}\" -c -o {}", ctx.selected_cl.exe.generic_string(), resp_args, in.generic_string(), output_dir.generic_string(), obj.generic_string());
				if (0 == std::system(cmd.c_str())) {
					std::puts(in.generic_string().c_str());
					objects.push_back(obj.generic_string());
				}
				else
					return false;
			}
		}

		return true;
		};

	if (!compile_sources("src"))
		return false;

	// Link objects
	std::println("Linking...");
	std::string const executable = fs::current_path().stem().string() + ".exe";
	auto const cmd = std::format("call \"{}\" {:s} -o {}/{}", ctx.selected_cl.exe.generic_string(), objects | std::views::join_with(' '), output_dir.generic_string(), executable);
	return (0 == std::system(cmd.c_str()));
}
