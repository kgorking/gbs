module;
#include <cstdio>
//#include <print>
export module mod6;
import mod5;

export void MyFunc6() {
	MyFunc5();
	std::puts("\tHello from module 'mod6'");
}
