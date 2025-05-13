module;
#include <print>
export module mod3;
import mod2;

export void MyFunc3() {
	std::println("\tHello from module 'mod3'");
}
