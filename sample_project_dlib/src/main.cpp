import std;

__declspec(dllexport) bool does_it_work_dynamic();

int main() {
	std::puts("GBS dlib test");
	std::println("  The rumours of dynamic linking working, are {}!", does_it_work_dynamic());
}
