module;
#include <filesystem>
#include <print>
export module cmd_run;
import context;

export bool cmd_run(context& ctx, std::string_view args) {
	std::string const executable = std::filesystem::current_path().stem().generic_string() + ".exe";
	std::println("<gbs> Running '{} {}'...", executable, args);
	std::string_view wsl = ctx.get_selected_compiler().is_wsl ? "wsl " : "";
	return 0 == std::system(std::format("{}\"{}/{}\" {}", wsl, ctx.output_dir().generic_string(), executable, args).c_str());
	//return 0 == std::system(std::format("cd {0} && {}\"{}\" {}", ctx.output_dir().generic_string(), wsl, executable, args).c_str());
}
