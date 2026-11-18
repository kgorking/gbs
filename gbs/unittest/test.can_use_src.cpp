#include "../inc/os.h"
import gs;

test can_use_static_lib = [] {
	test::is_true(is_target_triple_windows("-windows-"));
	};
