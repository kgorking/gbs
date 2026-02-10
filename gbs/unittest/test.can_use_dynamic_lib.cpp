#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#include <lib2.h>

TEST_CASE("test.can_use_dynamic_lib") {
	CHECK(true == lib2_does_it_work());
}
