import std;
import compiler;
import context;
import response;
import dep_scan;

import cmd_build;
import cmd_get_cl;
import cmd_enum_cl;

using namespace std::string_view_literals;
namespace fs = std::filesystem;


bool clean(context& ctx, std::string_view /*args*/) {
	std::println("<gbs> Cleaning...");

	// TODO args, clean=release
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
	std::string const executable = fs::current_path().stem().string() + ".exe";
	std::println("<gbs> Running '{}' ({})...\n", executable, ctx.output_config);

	if (!args.empty())
		ctx.output_config = args.substr(0, args.find_first_of(',', 0));

	if (ctx.output_config.empty()) {
		std::println("<gbs> Error: run : don't know what to run! Call 'run' after a compilation, or use eg. 'run=release' to run the release build.");
		exit(1);
	}

	return 0 == std::system(std::format("cd {} && {}", ctx.output_dir().generic_string(), executable).c_str());
}


bool cl(context& ctx, std::string_view args) {
	if (ctx.all_compilers.empty()) {
		fill_compiler_collection(ctx);
		if (ctx.all_compilers.empty()) {
			std::println("<gbs> Error: no compilers found while looking for '{}'.", args);
			exit(1);
		}
	}

	if (auto opt_cl = get_compiler(ctx, args); opt_cl) {
		ctx.selected_cl = *opt_cl;
		return true;
	} else {
		std::println("<gbs> Could not find compiler '{}'", args);
		return false;
	}
}


int main(int argc, char const* argv[]) {
	std::println("Gorking build system v0.09\n");

	context ctx;

	if (argc == 1) {
		fill_compiler_collection(ctx);
		ctx.select_first_compiler();
		return !cmd_build(ctx, "release");
	}

	static std::unordered_map<std::string_view, bool(*)(context&, std::string_view)> const commands = {
		{"enum_cl", cmd_enum_cl},
		{"get_cl", cmd_get_cl},
		{"cl", cl},
		{"clean", clean},
		{"build", cmd_build},
		{"run", run},
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
