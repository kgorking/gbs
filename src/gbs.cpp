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

int main(const int argc, char const* argv[], char const** envp) {
	auto ctx = context{ envp };

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

	for (auto const args = std::span(argv, static_cast<std::size_t>(argc)); std::string_view arg : args | std::views::drop(1)) {
		std::string_view const cmd = arg.substr(0, arg.find('='));
		if (!commands.contains(cmd)) {
			std::println("<gbs> Unknown command '{}', aborting\n", cmd);
			return 1;
		}

		arg.remove_prefix(cmd.size());
		arg.remove_prefix(!arg.empty() && arg.front() == '=');
		if (!commands.at(cmd)(ctx, arg)) {
			std::println("<gbs> aborting due to command failure.");
			return 1;
		}
	}

	return 0;
}
