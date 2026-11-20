#include <cstdio>
#include <string_view>
class context;

bool cmd_version(context& /*ctx*/, std::string_view /*args*/){
	std::puts("<gbs> Gorking build system v0.20 - Now compiles with multiple compilers at once!\n      Try it with 'cl=clang,gcc,msvc'");
	return true;
}