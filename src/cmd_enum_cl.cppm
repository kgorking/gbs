export module cmd_enum_cl;
import std;
import compiler;
import context;

export bool cmd_enum_cl(context& ctx, std::string_view /*args*/) {
	std::println("<gbs> Enumerating compilers:");

	fill_compiler_collection(ctx);

	for (auto& [k, v] : ctx.all_compilers) {
		std::println("<gbs>   {}: ", k);
		for (auto const& c : v) {
			std::println("<gbs>     {}.{} - {}", c.major, c.minor, c.dir.generic_string());
		}
	}

	return true;
}