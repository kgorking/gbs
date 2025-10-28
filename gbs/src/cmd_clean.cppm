export module cmd_clean;
import std;
import context;

export bool cmd_clean(context& ctx, std::string_view /*args*/) {
	std::println("<gbs> Cleaning...");

	// TODO args, clean=release
	std::error_code ec;
	std::filesystem::remove_all(ctx.get_gbs_out(), ec);
	if (ec) {
		std::println("<gbs> error : cleaning '{}' failed : {}", ctx.get_gbs_out().generic_string(), ec.message());
		return false;
	}

	std::filesystem::remove_all("gcm.cache", ec);
	if (ec) {
		std::println("<gbs> error : cleaning 'gcm.cache' failed : {}", ec.message());
		return false;
	}

	// TODO: move to own command
	/*std::filesystem::remove_all(ctx.get_gbs_internal(), ec);
	if (ec) {
		std::println("<gbs> error : cleaning '{}' failed : {}", ctx.get_gbs_internal().generic_string(), ec.message());
		return false;
	}*/

	return true;
}