#include <lib1.h>
import gs;

test can_use_static_lib = [] {
	test::is_true(lib1_does_it_work());
	};
