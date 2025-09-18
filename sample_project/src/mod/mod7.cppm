module;
#include <cstdio>
//#include <print>
export module mod7;
import mod6;

export void MyFunc7() {
	MyFunc6();
	std::puts("\tHello from module 'mod7'");
}
