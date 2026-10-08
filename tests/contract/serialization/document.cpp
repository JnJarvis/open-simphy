#include <algorithm>
#include <bit>
#include <catch2/catch_test_macros.hpp>
#include <limits>
#include <locale>
#include <opensim/serialization/document.hpp>
#include <sstream>

using namespace opensim;
namespace {
using Bytes = std::vector<std::byte>;
Bytes hex(const char *text) {
    std::istringstream stream(text);
    unsigned byte = 0;
    Bytes result;
    while (stream >> std::hex >> byte)
        result.push_back(static_cast<std::byte>(byte));
    return result;
}
Bytes empty() {
    return hex("4f 50 53 49 4d 44 4f 43 01 00 00 00 01 00 00 00 "
               "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 "
               "00 00 00 00 00 00 00 00 00 00 00 00 00 00 f0 bf");
}
Bytes one() {
    auto bytes = empty();
    bytes[16] = bytes[24] = std::byte{1};
    const auto record = hex("01 00 00 00 00 00 00 00 00 00 00 00 00 00 00 40 "
                            "00 00 00 00 00 00 f0 3f 00 00 00 00 00 00 00 c0 "
                            "00 00 00 00 00 00 00 00 00 00 00 00 00 00 e0 3f "
                            "00 00 00 00 00 00 d0 3f");
    bytes.insert(bytes.end(), record.begin(), record.end());
    return bytes;
}
void patch(Bytes &bytes, std::size_t offset, std::uint64_t value, std::size_t width = 8) {
    for (std::size_t i = 0; i < width; ++i) {
        bytes.at(offset + i) = static_cast<std::byte>(value % 256);
        value /= 256;
    }
}
template <class T> T take(const core::Result<T> &result) {
    REQUIRE(result.has_value());
    return *result.value();
}
void failure(const Bytes &bytes, core::Code code, const char *path,
             std::optional<core::EntityId> entity = {}) {
    const auto result = serialization::decode(bytes);
    REQUIRE(result.error());
    CHECK_FALSE(result.value());
    CHECK(result.error()->code == code);
    CHECK(result.error()->path == path);
    CHECK(result.error()->entity == entity);
    CHECK(result.error()->severity == core::Severity::error);
    CHECK_FALSE(result.error()->message.empty());
}
scene::ParticleFields fields() { return {2, {1, -2}, {0, .5}, .25}; }
} // namespace

TEST_CASE("F01 F02 F03 F23 literal native specimens and ownership") {
    const auto zero = take(scene::Document::create(1, {0, -1}, {}, {}));
    CHECK((take(serialization::encode(zero)) == empty()));
    const auto document = take(scene::Document::create(1, {0, -1}, {{{1}, fields()}}, {1}));
    CHECK((take(serialization::encode(document)) == one()));
    auto input = one();
    const auto decoded = take(serialization::decode(input));
    std::fill(input.begin(), input.end(), std::byte{0xff});
    CHECK(decoded == document);
    CHECK((take(serialization::encode(decoded)) == one()));
    struct Comma final : std::numpunct<char> {
        char do_decimal_point() const override { return ','; }
    };
    struct Restore {
        std::locale original = std::locale();
        ~Restore() { std::locale::global(original); }
    } restore;
    std::locale::global(std::locale(std::locale::classic(), new Comma));
    CHECK((take(serialization::encode(decoded)) == one()));
    CHECK(take(serialization::decode(one())) == document);
}
TEST_CASE("F04 F05 F06 full identities watermark and record normalization") {
    auto bytes = one();
    patch(bytes, 48, 9007199254740993ULL);
    patch(bytes, 24, UINT64_MAX);
    const auto document = take(serialization::decode(bytes));
    CHECK(document.particles()[0].id.value == 9007199254740993ULL);
    CHECK(document.high_water().value == UINT64_MAX);
    CHECK((take(serialization::encode(document)) == bytes));
    CHECK(scene::add_particle(document, fields()).error()->code == core::Code::id_exhausted);
    auto deleted = empty();
    patch(deleted, 24, 17);
    const auto restored = take(serialization::decode(deleted));
    CHECK(take(scene::add_particle(restored, fields())).affected_id->value == 18);
    auto unordered = one();
    const auto second = one();
    unordered.insert(unordered.end(), second.begin() + 48, second.end());
    patch(unordered, 16, 2, 4);
    patch(unordered, 24, 10);
    patch(unordered, 48, 9);
    patch(unordered, 104, 2);
    const auto sorted = take(serialization::decode(unordered));
    CHECK(sorted.particles()[0].id.value == 2);
    CHECK(sorted.particles()[1].id.value == 9);
    auto canonical = unordered;
    std::swap_ranges(canonical.begin() + 48, canonical.begin() + 104, canonical.begin() + 104);
    CHECK((take(serialization::encode(sorted)) == canonical));
}
TEST_CASE("F07 F08 F10 scalar bits and semantic rejection at every scalar offset") {
    const std::size_t offsets[] = {32, 40, 56, 64, 72, 80, 88, 96};
    const char *paths[] = {"gravity.x",
                           "gravity.y",
                           "particles.mass",
                           "particles.position.x",
                           "particles.position.y",
                           "particles.velocity.x",
                           "particles.velocity.y",
                           "particles.display_radius"};
    for (std::size_t i = 0; i < 8; ++i) {
        CAPTURE(offsets[i]);
        for (auto bits :
             {UINT64_C(1), UINT64_C(0x7fefffffffffffff), UINT64_C(0x8000000000000000)}) {
            auto bytes = one();
            patch(bytes, offsets[i], bits);
            if ((i == 2 || i == 7) && bits == UINT64_C(0x8000000000000000))
                failure(bytes, core::Code::invalid_data, paths[i], core::EntityId{1});
            else
                CHECK((take(serialization::encode(take(serialization::decode(bytes)))) == bytes));
        }
        for (auto bits : {UINT64_C(0x7ff8000000000001), UINT64_C(0x7ff0000000000000),
                          UINT64_C(0xfff0000000000000)}) {
            auto bytes = one();
            patch(bytes, offsets[i], bits);
            failure(bytes, core::Code::invalid_data, paths[i],
                    i < 2 ? std::nullopt : std::optional{core::EntityId{1}});
        }
    }
    for (const auto offset : {56U, 96U})
        for (const auto bits : {UINT64_C(0), UINT64_C(0xbff0000000000000)}) {
            auto bytes = one();
            patch(bytes, offset, bits);
            failure(bytes, core::Code::invalid_data,
                    offset == 56 ? "particles.mass" : "particles.display_radius",
                    core::EntityId{1});
        }
}
TEST_CASE("F09 scene identity diagnostics propagate unchanged") {
    auto bytes = one();
    patch(bytes, 48, 0);
    failure(bytes, core::Code::invalid_data, "particles.id", core::EntityId{0});
    const auto expected = scene::Document::create(1, {0, -1}, {{{0}, fields()}}, {1});
    CHECK(*serialization::decode(bytes).error() == *expected.error());
    bytes = one();
    patch(bytes, 24, 0);
    failure(bytes, core::Code::invalid_data, "high_water", core::EntityId{1});
    auto duplicate = one();
    duplicate.insert(duplicate.end(), bytes.begin() + 48, bytes.end());
    patch(duplicate, 16, 2, 4);
    failure(duplicate, core::Code::duplicate_id, "particles.id", core::EntityId{1});
}
TEST_CASE("F11 F12 F13 F20 truncated trailing and invalid magic bytes") {
    const auto specimen = one();
    for (std::size_t size = 0; size < specimen.size(); ++size)
        failure(Bytes(specimen.begin(), specimen.begin() + static_cast<std::ptrdiff_t>(size)),
                core::Code::invalid_data, "file.size");
    auto bytes = empty();
    bytes.push_back(std::byte{0});
    failure(bytes, core::Code::invalid_data, "file.size");
    bytes = empty();
    const auto extra = empty();
    bytes.insert(bytes.end(), extra.begin(), extra.end());
    failure(bytes, core::Code::invalid_data, "file.size");
    for (std::size_t i = 0; i < 8; ++i) {
        bytes = one();
        bytes[i] ^= std::byte{1};
        failure(bytes, core::Code::invalid_data, "file.magic");
    }
    bytes = one();
    patch(bytes, 16, 2, 4);
    failure(bytes, core::Code::invalid_data, "file.size");
}
TEST_CASE("F14 F15 F16 F17 F21 version count and validation precedence") {
    for (const auto value : {0U, 2U})
        for (const auto offset : {8U, 12U}) {
            auto bytes = empty();
            patch(bytes, offset, value, 4);
            failure(bytes, core::Code::unsupported_feature,
                    offset == 8 ? "file.version" : "schema_revision");
        }
    for (const auto value : {1U, 0x80000000U}) {
        auto bytes = empty();
        patch(bytes, 20, value, 4);
        failure(bytes, core::Code::unsupported_feature, "file.reserved");
    }
    for (const auto value : {4097U, UINT32_MAX}) {
        auto bytes = empty();
        patch(bytes, 16, value, 4);
        failure(bytes, core::Code::invalid_data, "file.count");
        patch(bytes, 8, 2, 4);
        failure(bytes, core::Code::unsupported_feature, "file.version");
        bytes[0] = std::byte{0};
        failure(bytes, core::Code::invalid_data, "file.magic");
    }
}
TEST_CASE("F18 F19 bounded capacity and F22 undetectable valid corruption") {
    std::vector<scene::Particle> particles;
    for (std::uint64_t id = 1; id <= 4096; ++id)
        particles.push_back({{id}, fields()});
    const auto full = take(scene::Document::create(1, {0, -1}, particles, {4096}));
    const auto bytes = take(serialization::encode(full));
    CHECK(bytes.size() == 229424);
    CHECK(take(serialization::decode(bytes)) == full);
    particles.push_back({{4097}, fields()});
    const auto over = take(scene::Document::create(1, {0, -1}, particles, {4097}));
    const auto encoded = serialization::encode(over);
    REQUIRE(encoded.error());
    CHECK(encoded.error()->code == core::Code::invalid_argument);
    CHECK(encoded.error()->path == "file.count");
    failure(Bytes(229425), core::Code::invalid_data, "file.size");
    auto corrupted = one();
    corrupted[56] = std::byte{1};
    CHECK(take(serialization::decode(corrupted)).particles()[0].fields.mass != 2);
}
