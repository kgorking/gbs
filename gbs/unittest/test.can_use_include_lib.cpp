#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

TEST_CASE("test.can_use_include_lib") {
	int constexpr some_val = 11;
	CHECK(some_val == 11);
}
