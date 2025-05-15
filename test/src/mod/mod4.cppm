module;
#include <print>
export module mod4;
import mod3;

export void MyFunc4() {
	MyFunc3();
	std::println("\tHello from module 'mod4'");
}
