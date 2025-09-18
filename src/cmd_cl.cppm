export module cmd_cl;
import std;
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
		return true;
	}
	else {
		std::println("<gbs> Could not find compiler '{}'", args);
		return false;
	}
}
