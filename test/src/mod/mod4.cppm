module;
#include <cstdio>
//#include <print>
export module mod4;
import mod3;

export void MyFunc4() {
	MyFunc3();
	std::puts("\tHello from module 'mod4'");
}
