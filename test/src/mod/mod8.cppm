module;
#include <cstdio>
//#include <print>
export module mod8;
import mod7;

export void MyFunc8() {
	MyFunc7();
	std::puts("\tHello from module 'mod8'");
}
