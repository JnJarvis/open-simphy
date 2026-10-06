#include <catch2/catch_test_macros.hpp>

#include <bit>
#include <cstdint>
#include <limits>
#include <numbers>
#include <span>

TEST_CASE("C++20 portable scalar and library baseline", "[scaffold]") {
    static_assert(sizeof(std::uint64_t) == 8);
    static_assert(std::numeric_limits<double>::is_iec559);
    static_assert(std::numeric_limits<double>::digits == 53);
    const int values[] = {2, 3, 5};
    const std::span<const int> view(values);
    REQUIRE(view.size() == 3);
    REQUIRE(view.back() == 5);
    REQUIRE(std::popcount(std::uint64_t{0b1011}) == 3);
    REQUIRE(std::numbers::pi > 3.14);
    REQUIRE(std::numbers::pi < 3.15);
}

// Hidden from the ordinary suite; run only by the isolated failure-reporting check.
TEST_CASE("intentional BUILD-002 failure probe", "[.failure-probe]") {
    FAIL("intentional BUILD-002 failure probe");
}
