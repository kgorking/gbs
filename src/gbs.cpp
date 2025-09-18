import std;
import compiler;
import context;
import monad;

import cmd_version;
import cmd_build;
import cmd_get_cl;
import cmd_enum_cl;
import cmd_clean;
import cmd_run;
import cmd_cl;
import cmd_ide;
import cmd_config;

int main(int argc, char const* argv[]) {
	context ctx;

	if (argc == 1) {
		ctx.fill_compiler_collection();
		ctx.select_first_compiler();
		cmd_config(ctx, "release");
		return !cmd_build(ctx, "");
	}

	static std::unordered_map<std::string_view, bool(*)(context&, std::string_view)> const commands = {
		{"version", cmd_version},
		{"enum_cl", cmd_enum_cl},
		{"get_cl", cmd_get_cl},
		{"cl", cmd_cl},
		{"config", cmd_config},
		{"clean", cmd_clean},
		{"build", cmd_build},
		{"run", cmd_run},
		{"ide", cmd_ide},
	};

	auto const args = std::span<char const*>(argv, argc);
	for (std::string_view arg : args | std::views::drop(1)) {
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
