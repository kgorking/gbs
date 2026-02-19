#include <cstdio>
#include <string_view>
class context;

bool cmd_version(context& /*ctx*/, std::string_view /*args*/){
	std::puts("<gbs> Gorking build system v0.19 - Now builds stuff in the correct order!");
	return true;
}