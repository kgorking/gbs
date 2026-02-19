#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#include "../inc/os.h"

TEST_CASE("test.can_use_static_lib") {
	CHECK(is_target_triple_windows("-windows-"));
}
