import gs;
#include <lib1.h>

test can_use_static_lib = [] {
	test::is_true(lib1_does_it_work());
	};
