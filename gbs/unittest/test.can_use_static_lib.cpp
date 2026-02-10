#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#include <lib1.h>

TEST_CASE("test.can_use_static_lib") {
	CHECK(true == lib1_does_it_work());
}
