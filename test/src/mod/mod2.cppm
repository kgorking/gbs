module;
#include <cstdio>
//#include <print>
export module mod2;
import mod1;

export void MyFunc2() {
	MyFunc1();
	std::puts("\tHello from module 'mod2'");
}
