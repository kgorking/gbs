#include <os.h>
import gs;

test can_use_src = [] {
	test::is_true(is_target_triple_windows("-windows-"));
	};
