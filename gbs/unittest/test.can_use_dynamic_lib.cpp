#include <lib2.h>
import gs;

test can_use_dynamic_lib = [] {
	test::is_true(lib2_does_it_work());
	};
