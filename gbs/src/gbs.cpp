#include <print>
#include <ranges>
#include <span>
#include <unordered_map>

import cmd_version;
import cmd_build;
import cmd_get_cl;
import cmd_enum_cl;
import cmd_clean;
import cmd_cl;
import cmd_ide;
import cmd_config;
import cmd_unittest;

import context;
import compiler;

int main(const int argc, char const* argv[], char const** envp) {
	auto ctx = context{ envp };

	if (argc == 1) {
		ctx.fill_compiler_collection();
		ctx.select_first_compiler();
		if(cmd_config(ctx, "debug"))
			if(cmd_build(ctx, ""))
				cmd_unittest(ctx, "");
		return 0;
	}

	auto const commands = std::unordered_map<std::string_view, bool(*)(context&, std::string_view)> {
		{"version", cmd_version},
		{"enum_cl", cmd_enum_cl},
		{"get_cl", cmd_get_cl},
		{"cl", cmd_cl},
		{"config", cmd_config},
		{"clean", cmd_clean},
		{"build", cmd_build},
		{"ide", cmd_ide},
		{"unittest", cmd_unittest}
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
