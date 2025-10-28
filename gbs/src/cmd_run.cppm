export module cmd_run;
import std;
import context;

export bool cmd_run(context& ctx, std::string_view args) {
	std::string const executable = std::filesystem::current_path().stem().generic_string() + ".exe";
	std::println("<gbs> Running '{} {}'...", executable, args);
	return 0 == std::system(std::format("cd {} && {} {}", ctx.output_dir().generic_string(), executable, args).c_str());
}
