#include <bit>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <cmath>
#include <iostream>
#include <opensim/serialization/document.hpp>
#include <random>

using namespace opensim;
namespace {
template <class T> T take(const core::Result<T> &result) {
    REQUIRE(result.has_value());
    return *result.value();
}
scene::Document sample(std::size_t count, std::mt19937_64 &random) {
    auto scalar = [&] {
        double value;
        do {
            value = std::bit_cast<double>(random());
        } while (!std::isfinite(value));
        return value;
    };
    std::vector<scene::Particle> particles;
    for (std::size_t i = 0; i < count; ++i)
        particles.push_back({{static_cast<std::uint64_t>(i + 1)},
                             {1, {scalar(), scalar()}, {scalar(), scalar()}, .25}});
    return take(scene::Document::create(1, {scalar(), scalar()}, std::move(particles),
                                        {static_cast<std::uint64_t>(count + 13)}));
}
} // namespace
TEST_CASE("Seeded arbitrary finite scalars round-trip with stable canonical bytes") {
    std::mt19937_64 random(0x1051);
    for (unsigned iteration = 0; iteration < 128; ++iteration) {
        const auto document = sample(iteration, random);
        const auto bytes = take(serialization::encode(document));
        const auto decoded = take(serialization::decode(bytes));
        CHECK(decoded == document);
        CHECK((take(serialization::encode(decoded)) == bytes));
        for (unsigned mutation = 0; mutation < 16; ++mutation) {
            auto mutated = bytes;
            mutated[static_cast<std::size_t>(random() % mutated.size())] ^= std::byte{0xff};
            const auto result = serialization::decode(mutated);
            if (result.value()) {
                const auto normalized = take(serialization::encode(*result.value()));
                CHECK((take(serialization::encode(take(serialization::decode(normalized)))) ==
                       normalized));
            } else {
                REQUIRE(result.error());
                CHECK(result.error()->severity == core::Severity::error);
            }
            CHECK((take(serialization::encode(document)) == bytes));
        }
    }
}
TEST_CASE("Native codec preliminary timing", "[.benchmark]") {
    std::mt19937_64 random(0x1051);
    for (const std::size_t count : {0U, 128U, 4096U}) {
        const auto document = sample(count, random);
        const auto bytes = take(serialization::encode(document));
        constexpr unsigned repeats = 1000;
        std::size_t observed = 0;
        const auto start = std::chrono::steady_clock::now();
        for (unsigned i = 0; i < repeats; ++i) {
            const auto encoded = serialization::encode(document);
            if (!encoded.value())
                FAIL("encode failed");
            observed += encoded.value()->size();
        }
        const auto middle = std::chrono::steady_clock::now();
        for (unsigned i = 0; i < repeats; ++i) {
            const auto decoded = serialization::decode(bytes);
            if (!decoded.value())
                FAIL("decode failed");
            observed += decoded.value()->particles().size();
        }
        const auto end = std::chrono::steady_clock::now();
        CHECK(observed == repeats * (bytes.size() + count));
        std::cout << "count=" << count << " bytes=" << bytes.size() << " encode_us="
                  << std::chrono::duration<double, std::micro>(middle - start).count() / repeats
                  << " decode_us="
                  << std::chrono::duration<double, std::micro>(end - middle).count() / repeats
                  << '\n';
    }
}
