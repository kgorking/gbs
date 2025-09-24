import std;
import mod9;

bool does_it_work();
__declspec(dllexport) bool does_it_work_dynamic();

short extra();

int main() {
	std::puts("Created by the Gorking build system");

	extra();
	MyFunc9();

	std::println("  The rumours of static linking working, are {}!", does_it_work());
	std::println("  The rumours of dynamic linking working, are {}!", does_it_work_dynamic());
}
