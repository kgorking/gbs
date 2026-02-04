module;
#include <string_view>
#include <print>
#include <iostream>

export module cmd_cl;
import context;

export bool cmd_cl(context& ctx, std::string_view args) {
	if (ctx.get_compiler_collection().empty()) {
		ctx.fill_compiler_collection();
		if (ctx.get_compiler_collection().empty()) {
			std::println("<gbs> Error: no compilers found while looking for '{}'.", args);
			return false;
		}
	}

	if (ctx.set_compiler(args)) {
		auto const& selected_cl = ctx.get_selected_compiler();
		std::println(std::cerr, "<gbs> Using compiler '{} {}.{}.{}'", selected_cl.name, selected_cl.major, selected_cl.minor, selected_cl.patch);
		return true;
	}
	else {
		std::println("<gbs> Could not find compiler '{}'", args);
		return false;
	}
}
