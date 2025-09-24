module;
#include <cstdio>
//#include <print>
export module mod5;
import mod4;

export void MyFunc5() {
	MyFunc4();
	std::puts("\tHello from module 'mod5'");
}
