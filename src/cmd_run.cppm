export module cmd_run;
import std;
import context;

export bool cmd_run(context& ctx, std::string_view args) {
	std::string const executable = std::filesystem::current_path().stem().string() + ".exe";
	std::println("<gbs> Running '{}' ({})...\n", executable, ctx.config);

	if (!args.empty())
		ctx.config = args.substr(0, args.find_first_of(',', 0));

	if (ctx.config.empty()) {
		std::println("<gbs> Error: run : don't know what to run! Call 'run' after a compilation, or use eg. 'run=release' to run the release build.");
		exit(1);
	}

	return 0 == std::system(std::format("cd {} && {}", ctx.output_dir().generic_string(), executable).c_str());
}
