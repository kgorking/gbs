import std;
import mod9;
import test.will_it_work;

short extra();

int main() {
	std::puts("Created by the Gorking build system");
	extra();
	MyFunc9();
	std::println("Does linking work? {}!", does_it_work());
}
