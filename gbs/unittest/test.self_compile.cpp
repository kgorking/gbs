#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

TEST_CASE("test") {
	int constexpr some_val = 11;
	CHECK(some_val == 11);
}
