export module cmd_cl;
import std;
import context;

export bool cmd_cl(context& ctx, std::string_view args) {
	if (ctx.all_compilers.empty()) {
		fill_compiler_collection(ctx);
		if (ctx.all_compilers.empty()) {
			std::println("<gbs> Error: no compilers found while looking for '{}'.", args);
			return false;
		}
	}

	if (auto opt_cl = get_compiler(ctx, args); opt_cl) {
		ctx.selected_cl = *opt_cl;
		return true;
	}
	else {
		std::println("<gbs> Could not find compiler '{}'", args);
		return false;
	}
}
