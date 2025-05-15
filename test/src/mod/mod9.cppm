module;
#include <print>
export module mod9;
import mod8;

export void MyFunc9() {
	MyFunc8();
	std::println("\tHello from module 'mod9'");
}
