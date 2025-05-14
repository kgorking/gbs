module;
#include <print>
export module mod4;
import mod3;

export void MyFunc4() {
	std::println("\tHello from module 'mod4'");
}
