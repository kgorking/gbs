#include "../inc/context.h"
#include <cstdio>
#include <string_view>

bool cmd_version(context& /*ctx*/, std::string_view /*args*/){
	std::puts("<gbs> Gorking build system v0.18.0 - Now on linux! ");
	return true;
}