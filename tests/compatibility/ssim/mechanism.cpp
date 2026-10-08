#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <miniz.h>
#include <opensim/compat/mechanical_source.hpp>
using namespace opensim;
namespace {
const std::string body =
    R"(<Body Id="a" Name="ball"><Transform><Translation x="0" y="0"/><Rotation>0</Rotation></Transform><Mass><LocalCenter x="0" y="0"/><Type>FIXED_ANGULAR_VELOCITY</Type><Mass>1</Mass><Inertia>.125</Inertia></Mass><Fixtures><Fixture><Shape xsi:type="Circle"><LocalCenter x="0" y="0"/><Radius>.5</Radius></Shape><Friction>.3</Friction><Restitution>1</Restitution></Fixture></Fixtures><Velocity x="0" y="0"/><AngularVelocity>0</AngularVelocity></Body>)";
compat::Project project(const std::string &script, const std::string &gui = "") {
    std::string xml = "<Simulation xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance' "
                      "version='4.1'><World><Gravity x='0' y='-9.8'/><Bodies>" +
                      body + "</Bodies><ScriptManager><Script><![CDATA[" + script +
                      "]]></Script></ScriptManager><GuiManager><GuiXML><![CDATA[" + gui +
                      "]]></GuiXML></GuiManager></World></Simulation>";
    mz_zip_archive z{};
    REQUIRE(mz_zip_writer_init_heap(&z, 0, 0));
    REQUIRE(
        mz_zip_writer_add_mem(&z, "simulation.xml", xml.data(), xml.size(), MZ_BEST_COMPRESSION));
    void *data = nullptr;
    std::size_t n = 0;
    REQUIRE(mz_zip_writer_finalize_heap_archive(&z, &data, &n));
    std::vector<std::uint8_t> bytes(static_cast<std::uint8_t *>(data),
                                    static_cast<std::uint8_t *>(data) + n);
    mz_free(data);
    mz_zip_writer_end(&z);
    auto parsed = compat::inspect_ssim(bytes);
    REQUIRE(parsed.value());
    return *parsed.value();
}
} // namespace
TEST_CASE("Source script creates actual copied circles, colors, joints and actions",
          "[compatibility][mechanism]") {
    auto p = project(
        R"(var ball=World.getBody('ball');function reset(){World.clear();var v=new Vector2(0,0);for(let i=0;i<5;i++){let b=World.createCopy(ball);b.setFillColor(new Color(i==0?'red':'green'));b.setPosition(v.add(1.01,0));World.addDistanceJoint(b,null,v,v.sum(0,3));}}reset();)",
        R"GUI(<desktop><button text="Reset" action="reset()" rectbounds="20,20,100,32"/><textarea text="A test description" rectbounds="20,80,200,50"/></desktop>)GUI");
    auto result = compat::MechanicalSource::create(p);
    REQUIRE(result.value());
    auto s = *result.value();
    REQUIRE(s.definition().bodies.size() == 6);
    REQUIRE(s.definition().links.size() == 5);
    REQUIRE(s.definition().bodies[1].center == math::Vec2{1.01, 0});
    REQUIRE(s.styles().at({2}).fill == scene::Color{1, 0, 0, 1});
    REQUIRE(s.widgets().size() == 2);
    REQUIRE(s.apply_action("reset()").has_value());
    REQUIRE(s.definition().bodies.size() == 6);
    REQUIRE(s.definition().links.size() == 5);
    auto old = s.definition().bodies[1].center;
    REQUIRE(s.apply_action("World.clearAll()").error());
    REQUIRE(s.definition().bodies[1].center == old);
}
TEST_CASE("Bounded source scripts reject unknown APIs and runaway code",
          "[compatibility][mechanism]") {
    REQUIRE(compat::MechanicalSource::create(project("App.readHttpRequest('https://example.com')"))
                .error());
    auto start = std::chrono::steady_clock::now();
    auto r = compat::MechanicalSource::create(project("while(true){}"));
    REQUIRE(r.error());
    REQUIRE(std::chrono::steady_clock::now() - start < std::chrono::seconds(2));
    REQUIRE(compat::MechanicalSource::create(
                project("var b=World.getBody('ball');for(let i=0;i<300;i++)World.createCopy(b)"))
                .error());
    REQUIRE(compat::MechanicalSource::create(
                project("var b=World.getBody('ball');b.setPosition(new Vector2(Infinity,0))"))
                .error());
    REQUIRE(compat::MechanicalSource::create(
                project("let a=[];while(true)a.push('a'.repeat(1000000));"))
                .error());
}
