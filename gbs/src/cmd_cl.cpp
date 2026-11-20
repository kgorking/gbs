#include "../inc/commands.h"
#include "../inc/compiler.h"
#include "../inc/context.h"
#include <print>

bool cmd_cl(context& ctx, std::string_view args) {
	if (ctx.get_compiler_collection().empty()) {
		ctx.fill_compiler_collection();
		if (ctx.get_compiler_collection().empty()) {
			std::println(std::cerr, "<gbs> Error: no compilers found while looking for '{}'.", args);
			return false;
		}
	}

	if (ctx.set_compiler(args)) {
		for (compiler const& cl : ctx.get_selected_compilers()) {
			std::println("<gbs> Using compiler '{} {}.{}.{}'", cl.name, cl.major, cl.minor, cl.patch);
		}
		return true;
	}
	else {
		std::println(std::cerr, "<gbs> Could not find compiler '{}'", args);
		return false;
	}
}
