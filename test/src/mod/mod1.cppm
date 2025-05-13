module;
#include <print>
export module mod1;
import mod0;

export void MyFunc1() {
	std::println("\tHello from module 'mod1'");
}
