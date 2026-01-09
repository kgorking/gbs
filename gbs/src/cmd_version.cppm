module;
#include <print>
export module cmd_version;
import context;

export bool cmd_version(context& /*ctx*/, std::string_view /*args*/){
	std::println("<gbs> Gorking build system v0.18.0 - Now on linux!");
	return true;
}