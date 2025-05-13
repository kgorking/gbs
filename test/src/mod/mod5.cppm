module;
#include <print>
export module mod5;
import mod4;

export void MyFunc5() {
	std::println("\tHello from module 'mod5'");
}
