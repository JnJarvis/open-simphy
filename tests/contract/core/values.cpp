#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <opensim/core/values.hpp>
#include <type_traits>
using namespace opensim::core;

TEST_CASE("C01-C08 identity allocation contract") {
    constexpr auto max = std::numeric_limits<std::uint64_t>::max();
    CHECK_FALSE(EntityId{0}.valid());
    CHECK(EntityId{1}.valid());
    CHECK(EntityId{max}.valid());
    const EntityId copy = EntityId{42};
    CHECK(copy == EntityId{42});
    CHECK(copy < EntityId{43});
    IdAllocator a;
    CHECK(a.next().value()->value == 1);
    CHECK(a.next().value()->value == 2);
    REQUIRE(a.reserve_through({10}).has_value());
    CHECK(a.next().value()->value == 11);
    REQUIRE(a.reserve_through({5}).has_value());
    CHECK(a.next().value()->value == 12);
    IdAllocator b;
    REQUIRE(b.reserve_through({7}).has_value());
    CHECK(b.reserve_through({0}).error()->code == Code::invalid_argument);
    CHECK(b.next().value()->value == 8);
    REQUIRE(b.reserve_through({max - 1}).has_value());
    CHECK(b.next().value()->value == max);
    CHECK(b.next().error()->code == Code::id_exhausted);
    CHECK(b.high_water().value == max);
    IdAllocator c, d;
    CHECK(c.next().value()->value == 1);
    CHECK(d.next().value()->value == 1);
    REQUIRE(c.reserve_through({7}).has_value());
    REQUIRE(c.reserve_through({7}).has_value());
    CHECK(c.next().value()->value == 8);
}
TEST_CASE("C09-C14 result ownership and diagnostics") {
    auto zero = Result<int>::success(0);
    REQUIRE(zero.has_value());
    CHECK(*zero.value() == 0);
    CHECK(zero.error() == nullptr);
    auto owned = [] {
        Diagnostic d{Code::invalid_argument, Severity::error, "bad mass", EntityId{42},
                     "body.mass"};
        auto source = Result<int>::failure(d);
        return Result<int>(source);
    }();
    REQUIRE(owned.error());
    CHECK(owned.value() == nullptr);
    CHECK(owned.error()->message == "bad mass");
    CHECK(owned.error()->entity == EntityId{42});
    CHECK(owned.error()->path == "body.mass");
    auto propagated = Result<void>::failure(*owned.error());
    CHECK(*propagated.error() == *owned.error());
    CHECK(Result<void>::success().has_value());
    CHECK(Result<void>::success().error() == nullptr);
    static_assert(!std::is_default_constructible_v<Result<int>>);
    static_assert(!std::is_copy_constructible_v<Result<std::unique_ptr<int>>>);
    auto first = Result<std::unique_ptr<int>>::success(std::make_unique<int>(17));
    auto moved = std::move(first);
    REQUIRE(moved.value());
    CHECK(**moved.value() == 17);
    auto warning = Result<int>::failure({Code::invalid_data, Severity::warning, "warning", {}, {}});
    REQUIRE(warning.error());
    CHECK(warning.error()->code == Code::invalid_argument);
    CHECK(warning.error()->severity == Severity::error);
}
