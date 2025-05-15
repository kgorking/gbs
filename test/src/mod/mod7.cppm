module;
#include <print>
export module mod7;
import mod6;

export void MyFunc7() {
	MyFunc6();
	std::println("\tHello from module 'mod7'");
}
