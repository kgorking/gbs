#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
import os;

TEST_CASE("test.can_use_static_lib") {
	CHECK(is_target_triple_windows("-windows-"));
}
