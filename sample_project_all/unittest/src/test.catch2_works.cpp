#include <catch2/catch_test_macros.hpp>
import std;


// A bunch of tests to ensure that the component_pool behaves as expected
TEST_CASE("CASE", "[case]") {
	SECTION("SECTION") {
		int const some_val = 11;
		REQUIRE(some_val == 11);
	}
}
