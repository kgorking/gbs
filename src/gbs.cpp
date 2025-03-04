#include <filesystem>
#include <fstream>
#include <string_view>
#include <ranges>
#include <print>
#include <span>
#include "compiler.h"
#include "context.h"
#include "response.h"

using namespace std::string_view_literals;
namespace fs = std::filesystem;



bool enum_cl(context& ctx, std::string_view /*args*/) {
	std::println("<gbs> Enumerating compilers:");

	fill_compiler_collection(ctx);

	for(auto& [k, v] : ctx.all_compilers) {
		std::println("<gbs> {}: ", k);
		for (auto const &c : v) {
			std::println("<gbs>   {}.{} - {}", c.major, c.minor, c.dir.generic_string());
		}
	}

	return true;
}


bool build(context& ctx, std::string_view args) {
	if (!fs::exists("src/")) {
		std::println("<gbs> Error: no 'src' directory found at '{}'", fs::current_path().generic_string());
		return false;
	}

	// Default build config is 'release'
	if (args.empty())
		args = "release";

	extern bool build_msvc(context & ctx, std::string_view args);
	return build_msvc(ctx, args);
}

bool clean(context& ctx, std::string_view /*args*/) {
	// TODO args, clean=release
	std::println("<gbs> Cleaning...");
	std::error_code ec;
	std::filesystem::remove_all(ctx.gbs_out, ec);
	if (ec) {
		std::println("<gbs> Error: {}", ec.message());
		return false;
	}

	// TODO: move to own command
	std::filesystem::remove_all(ctx.gbs_internal, ec);
	if (ec) {
		std::println("<gbs> Error: {}", ec.message());
		return false;
	}

	return true;
}

bool run(context& ctx, std::string_view args) {
	if (!args.empty())
		ctx.output_config = args.substr(0, args.find_first_of(',', 0));

	if (ctx.output_config.empty()) {
		std::println("<gbs> Error: run : don't know what to run! Call 'run' after a compilation, or use eg. 'run=release' to run the release build.");
		exit(1);
	}

	std::string const executable = fs::current_path().stem().string() + ".exe";
	std::println("<gbs> Running '{}' ({})...\n", executable, ctx.output_config);
	return 0 == std::system(std::format("cd {} && {}", ctx.output_dir().generic_string(), executable).c_str());
}


bool cl(context& ctx, std::string_view args) {
	if (ctx.all_compilers.empty()) {
		fill_compiler_collection(ctx);
		if (ctx.all_compilers.empty()) {
			std::println("<gbs> Error: no compilers found.");
			exit(1);
		}
	}

	ctx.selected_cl = get_compiler(ctx, args);
	std::println("<gbs> Using compiler '{} v{}.{}'", ctx.selected_cl.name, ctx.selected_cl.major, ctx.selected_cl.minor);
	return true;
}


int main(int argc, char const* argv[]) {
	std::println("Gorking build system v0.05\n");

	context ctx;

	if (argc == 1) {
		enum_cl(ctx, {});
		ctx.select_first_compiler();
		std::println("<gbs> Using compiler '{} v{}.{}'", ctx.selected_cl.name, ctx.selected_cl.major, ctx.selected_cl.minor);
		return !build(ctx, "release");
	}

	static std::unordered_map<std::string_view, bool(*)(context&, std::string_view)> const commands = {
		{"build", build},
		{"clean", clean},
		{"run", run},
		{"enum_cl", enum_cl},
		{"cl", cl},
	};

	auto const args = std::span<char const*>(argv, argc);
	for (int i = 1; i < argc; i++) {
		std::string_view arg{ argv[i] };

		std::string_view const left = arg.substr(0, arg.find('='));
		if (!commands.contains(left)) {
			std::println("<gbs> Unknown command '{}', aborting\n", left);
			return 1;
		}

		arg.remove_prefix(left.size());
		arg.remove_prefix(!arg.empty() && arg.front() == '=');
		if (!commands.at(left)(ctx, arg)) {
			std::println("<gbs> aborting due to command failure.");
			return 1;
		}
	}

	return 0;
}
