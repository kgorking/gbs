module;
#include <cstdio>
//#include <print>
export module mod3;
import mod2;

export void MyFunc3() {
	MyFunc2();
	std::puts("\tHello from module 'mod3'");
}
