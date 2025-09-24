import std;
extern bool does_it_work();

int main() {
	std::puts("GBS slib test");
	std::println("  The rumours of static linking working, are {}!", does_it_work());
}
