import std;
import mod9;
import test.will_it_work;

short extra();

int main() {
	std::puts("Created by the Gorking build system");

	extra();
	MyFunc9();

	std::println("  The rumours of linking working, are {}!", does_it_work());
}
