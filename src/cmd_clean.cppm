export module cmd_clean;
import std;
import context;

export bool cmd_clean(context& ctx, std::string_view /*args*/) {
	std::println("<gbs> Cleaning...");

	// TODO args, clean=release
	std::error_code ec;
	std::filesystem::remove_all(ctx.gbs_out, ec);
	if (ec) {
		std::println("<gbs> error : clean failed : {}", ec.message());
		return false;
	}

	// TODO: move to own command
	std::filesystem::remove_all(ctx.gbs_internal, ec);
	if (ec) {
		std::println("<gbs> error : clean internals failed : {}", ec.message());
		return false;
	}

	return true;
}