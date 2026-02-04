module;
#include <string_view>
#include <print>
export module cmd_enum_cl;
import context;

export bool cmd_enum_cl(context& ctx, std::string_view /*args*/) {
	std::println("<gbs> Enumerating compilers:");

	ctx.fill_compiler_collection();

	//[[gsl::suppress("gsl.view")]]
	for (auto const& [name, compilers] : ctx.get_compiler_collection()) {
		std::println("<gbs>   {}: ", name);
		for (auto const& c : compilers) {
			std::print("<gbs>     {}.{}.{} - {}", c.major, c.minor, c.patch, c.dir.generic_string());
			if (c.wsl.has_value()) {
				std::println(" [wsl:{}]", c.wsl.value());
			}
			else {
				std::println();
			}
		}
	}

	return true;
}