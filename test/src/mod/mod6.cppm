module;
#include <print>
export module mod6;
import mod5;

export void MyFunc6() {
	MyFunc5();
	std::println("\tHello from module 'mod6'");
}
