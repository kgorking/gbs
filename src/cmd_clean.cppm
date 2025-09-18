export module cmd_clean;
import std;
import context;

export bool cmd_clean(context& ctx, std::string_view /*args*/) {
	std::println("<gbs> Cleaning...");

	// TODO args, clean=release
	std::error_code ec;
	std::filesystem::remove_all(ctx.gbs_out, ec);
	if (ec) {
		std::println("<gbs> error : cleaning '{}' failed : {}", ctx.gbs_out.string(), ec.message());
		return false;
	}

	std::filesystem::remove_all("gcm.cache", ec);
	if (ec) {
		std::println("<gbs> error : cleaning 'gcm.cache' failed : {}", ec.message());
		return false;
	}

	// TODO: move to own command
	std::filesystem::remove_all(ctx.gbs_internal, ec);
	if (ec) {
		std::println("<gbs> error : cleaning '{}' failed : {}", ctx.gbs_internal.string(), ec.message());
		return false;
	}

	return true;
}