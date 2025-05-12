import std;
import compiler;
import context;
import response;
import dep_scan;

using namespace std::string_view_literals;
namespace fs = std::filesystem;

bool get_cl(context&, std::string_view args);


bool enum_cl(context& ctx, std::string_view /*args*/) {
	std::println("<gbs> Enumerating compilers:");

	fill_compiler_collection(ctx);

	for(auto& [k, v] : ctx.all_compilers) {
		std::println("<gbs>   {}: ", k);
		for (auto const &c : v) {
			std::println("<gbs>     {}.{} - {}", c.major, c.minor, c.dir.generic_string());
		}
	}

	return true;
}


bool build(context& ctx, std::string_view args) {
	std::println("<gbs> Building...");

	if (!fs::exists("src/")) {
		std::println("<gbs> Error: no 'src' directory found at '{}'", fs::current_path().generic_string());
		return false;
	}

	// Default build config is 'release'
	if (args.empty())
		args = "release";

	// Ensure the needed response files are present
	init_response_files(ctx);
	check_response_files(ctx, args);

	if (ctx.selected_cl.name == "msvc") {
		extern bool build_msvc(context & ctx, std::string_view args);
		return build_msvc(ctx, args);
	} if (ctx.selected_cl.name.starts_with("clang")) {
		extern bool build_clang(context & ctx, std::string_view args);
		return build_clang(ctx, args);
	//} if (ctx.selected_cl.name == "gcc") {
	//	extern bool build_gcc(context & ctx, std::string_view args);
	//	return build_gcc(ctx, args);
	} else {
		std::println("<gbs> INTERNAL : not implemented yet");
		return false;
	}
}

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
	std::println("<gbs> cl : searchin for compiler '{}'.", args);
	if (ctx.all_compilers.empty()) {
		fill_compiler_collection(ctx);
		if (ctx.all_compilers.empty()) {
			std::println("<gbs>   Error: no compilers found.");
			exit(1);
		}
	}

	if (auto opt_cl = get_compiler(ctx, args); opt_cl) {
		ctx.selected_cl = *opt_cl;
		std::println("<gbs>   Using compiler '{} {}.{}'", ctx.selected_cl.name, ctx.selected_cl.major, ctx.selected_cl.minor);
		return true;
	} else {
		std::println("<gbs>   Could not find compiler '{}'", args);
		return false;
	}
}


int main(int argc, char const* argv[]) {
	std::println("Gorking build system v0.09\n");

	auto test = detect_module_dependencies("./src");
	for (auto &v : test)
		std::println("{} - {}", v.first.generic_string(), v.second);
	return 0;

	context ctx;

	if (argc == 1) {
		enum_cl(ctx, {});
		ctx.select_first_compiler();
		std::println("<gbs> Using compiler '{} v{}.{}'", ctx.selected_cl.name, ctx.selected_cl.major, ctx.selected_cl.minor);
		return !build(ctx, "release");
	}

	static std::unordered_map<std::string_view, bool(*)(context&, std::string_view)> const commands = {
		{"enum_cl", enum_cl},
		{"get_cl", get_cl},
		{"cl", cl},
		{"clean", clean},
		{"build", build},
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
