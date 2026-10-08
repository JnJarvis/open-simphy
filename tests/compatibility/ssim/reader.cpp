#include "independent.hpp"
#include <algorithm>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <miniz.h>
#include <opensim/compat/ssim.hpp>
using namespace opensim;
namespace {
std::vector<std::uint8_t> archive(const std::vector<std::pair<std::string, std::string>> &members,
                                  bool deflate = false) {
    mz_zip_archive zip{};
    REQUIRE(mz_zip_writer_init_heap(&zip, 0, 0));
    std::vector<std::pair<std::string, std::string>> rename;
    for (const auto &[name, content] : members) {
        // The writer rejects some hostile names itself. Build a legal name then
        // mutate both ZIP headers so the reader sees literal hostile bytes.
        const bool hostile = name.find_first_of("/\\:") != std::string::npos;
        const std::string stored = hostile ? std::string(name.size(), 'Q') : name;
        REQUIRE(mz_zip_writer_add_mem(&zip, stored.c_str(), content.data(), content.size(),
                                      deflate ? MZ_BEST_COMPRESSION : 0));
        if (hostile)
            rename.emplace_back(stored, name);
    }
    void *data = nullptr;
    size_t length = 0;
    REQUIRE(mz_zip_writer_finalize_heap_archive(&zip, &data, &length));
    const auto *bytes = static_cast<const std::uint8_t *>(data);
    std::vector<std::uint8_t> result(bytes, bytes + length);
    for (const auto &[stored, name] : rename) {
        auto begin = result.begin();
        while (true) {
            auto found = std::search(begin, result.end(), stored.begin(), stored.end());
            if (found == result.end())
                break;
            std::copy(name.begin(), name.end(), found);
            begin = found + static_cast<std::ptrdiff_t>(name.size());
        }
    }
    mz_free(data);
    mz_zip_writer_end(&zip);
    return result;
}
std::string scene(const std::string &content = "") {
    return "<Simulation xmlns:q='http://www.w3.org/2001/XMLSchema-instance' version='4.2' "
           "title='Real shapes'><World>" +
           content + "</World></Simulation>";
}
} // namespace
TEST_CASE("SSIM stored and deflated archives preserve source and inventory nested geometry") {
    const auto xml =
        scene("<Bodies><Group><Body Name='rectangle'><Transform><Translation x='2' "
              "y='3'/><Rotation>90</Rotation></Transform><Fixtures><Fixture><Shape "
              "q:type='Rectangle'><LocalCenter x='0' "
              "y='0'/><Width>2</Width><Height>4</Height></Shape></Fixture><Fixture><Shape "
              "q:type='Ring'><Radius>1</Radius></Shape></Fixture></Fixtures></Body></Group></"
              "Bodies><Joints><Joint "
              "q:type='DistanceJoint'/></Joints><ScriptManager><Script>run()</Script><Script>  "
              "</Script></ScriptManager><UnknownFeature/>");
    for (bool compressed : {false, true}) {
        auto bytes = archive({{"simulation.xml", xml}, {"texture.png", "asset bytes"}}, compressed);
        auto result = compat::inspect_ssim(bytes);
        REQUIRE(result.value());
        auto project = *result.value();
        CHECK(project.source == bytes);
        bytes.clear();
        CHECK(project.title == "Real shapes");
        CHECK(project.version == "4.2");
        CHECK(project.elements.at("Body") == 1);
        CHECK(project.elements.at("Fixture") == 2);
        CHECK(project.elements.at("UnknownFeature") == 1);
        CHECK(project.joints.at("DistanceJoint") == 1);
        CHECK(project.shapes.at("Ring") == 1);
        CHECK(project.scripts == 1);
        REQUIRE(project.outlines.size() == 1);
        REQUIRE(project.outlines[0].points.size() == 4);
        CHECK(project.outlines[0].points[0].x == Catch::Approx(4));
        CHECK(project.outlines[0].points[0].y == Catch::Approx(2));
        CHECK(project.diagnostics.size() >= 3);
    }
}
TEST_CASE("SSIM rejects malformed archives names missing XML and broken CRC") {
    CHECK(compat::inspect_ssim({}).error());
    for (const auto &name : {"../simulation.xml", "/simulation.xml", "a\\simulation.xml",
                             "a/./simulation.xml", "C:simulation.xml"})
        CHECK(compat::inspect_ssim(archive({{name, scene()}})).error());
    CHECK(compat::inspect_ssim(archive({{"other.xml", scene()}})).error());
    CHECK(compat::inspect_ssim(archive({{"simulation.xml", scene()}, {"simulation.xml", scene()}}))
              .error());
    auto bytes = archive({{"simulation.xml", scene()}});
    auto cut = bytes;
    cut.resize(cut.size() - 10);
    CHECK(compat::inspect_ssim(cut).error());
    constexpr std::string_view marker = "Real shapes";
    const auto found = std::search(bytes.begin(), bytes.end(), marker.begin(), marker.end());
    REQUIRE(found != bytes.end());
    *found = 'X';
    CHECK(compat::inspect_ssim(bytes).error());
}
TEST_CASE("SSIM rejects hostile XML and unsupported versions with bounded work") {
    for (const auto &xml : {"<Simulation version='5.0'/>", "<Other/>",
                            "<Simulation version='4.2'><broken></Simulation>",
                            "<!DOCTYPE Simulation [<!ENTITY a 'boom'>]><Simulation version='4.2'/>",
                            "<Simulation version='4.2'/><Simulation version='4.2'/>"})
        CHECK(compat::inspect_ssim(archive({{"simulation.xml", xml}})).error());
    std::string deep;
    for (int i = 0; i < 130; ++i)
        deep += "<n>";
    for (int i = 0; i < 130; ++i)
        deep += "</n>";
    CHECK(compat::inspect_ssim(archive({{"simulation.xml", scene(deep)}})).error());
    std::string many;
    for (int i = 0; i < 200001; ++i)
        many += "<n/>";
    CHECK(compat::inspect_ssim(archive({{"simulation.xml", scene(many)}})).error());
    std::string huge(8 * 1024 * 1024 + 1, 'x');
    CHECK(compat::inspect_ssim(archive({{"simulation.xml", huge}})).error());
    auto null = scene();
    null[5] = '\0';
    CHECK(compat::inspect_ssim(archive({{"simulation.xml", null}})).error());
}

TEST_CASE("SSIM accepts independently produced ZIP and rejects forbidden metadata") {
    auto independent = compat::inspect_ssim(independent_zip);
    REQUIRE(independent.value());
    CHECK(independent.value()->title == "Independent Python specimen");
    auto bytes = archive({{"simulation.xml", scene()}});
    const std::uint8_t central[]{0x50, 0x4b, 0x01, 0x02};
    const auto found =
        std::search(bytes.begin(), bytes.end(), std::begin(central), std::end(central));
    REQUIRE(found != bytes.end());
    const auto offset = static_cast<std::size_t>(found - bytes.begin());
    auto encrypted = bytes;
    encrypted[offset + 8] |= 1;
    CHECK(compat::inspect_ssim(encrypted).error());
    auto linked = bytes;
    linked[offset + 40] = 0xff;
    linked[offset + 41] = 0xa1;
    CHECK(compat::inspect_ssim(linked).error());
    auto method = bytes;
    method[offset + 10] = 99;
    CHECK(compat::inspect_ssim(method).error());
    std::vector<std::uint8_t> oversized(compat::max_ssim_bytes + 1);
    CHECK(compat::inspect_ssim(oversized).error());
}

TEST_CASE("SSIM invalid UTF8 and excessive expansion are rejected") {
    std::string invalid = scene();
    invalid[5] = static_cast<char>(0xff);
    CHECK(compat::inspect_ssim(archive({{"simulation.xml", invalid}})).error());
    CHECK(compat::inspect_ssim(
              archive({{"simulation.xml", scene()}, {"asset", std::string(4 * 1024 * 1024, 'x')}},
                      true))
              .error());
}
