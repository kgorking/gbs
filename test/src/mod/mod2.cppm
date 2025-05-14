module;
#include <print>
export module mod2;
import mod1;

export void MyFunc2() {
	std::println("\tHello from module 'mod2'");
}
