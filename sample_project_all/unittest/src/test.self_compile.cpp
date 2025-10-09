//#include <catch2/catch_all.hpp>
//#include <lib2.hpp>
#include <catch2/catch_test_macros.hpp>
import std;
import helper;

TEST_CASE("CASE", "[case]") {
	SECTION("SECTION") {
		int const some_val = 11;
		REQUIRE(some_val == 11);
	}
}
