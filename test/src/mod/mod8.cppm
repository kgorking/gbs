module;
#include <print>
export module mod8;
import mod7;

export void MyFunc8() {
	std::println("\tHello from module 'mod8'");
}
