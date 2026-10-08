#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <cmath>
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

TEST_CASE("Script distance anchors use the body's rotated local frame",
          "[compatibility][mechanism]") {
    auto result = compat::MechanicalSource::create(
        project("var b=World.getBody('ball');b.d.angle=Math.PI/2;World.addDistanceJoint(b,null,new "
                "Vector2(1,0),new Vector2(1,3));"));
    REQUIRE(result.value());
    const auto &j = result.value()->definition().links.front();
    REQUIRE(std::abs(j.local_a.x) < 1e-12);
    REQUIRE(std::abs(j.local_a.y + 1) < 1e-12);
    REQUIRE(j.local_b == math::Vec2{1, 3});
}

namespace {
compat::Project rigid_project(std::string shape, std::string controllers = "",
                              std::string script = "", std::string gui = "", double com = 0,
                              double angle = 0) {
    std::string b = body;
    const auto mass_center = b.find("<LocalCenter");
    const auto mass_end = b.find("/>", mass_center);
    b.replace(mass_center, mass_end + 2 - mass_center,
              "<LocalCenter x=\"" + std::to_string(com) + "\" y=\"0\"/>");
    b.replace(b.find("<Rotation>0</Rotation>"), 22,
              "<Rotation>" + std::to_string(angle) + "</Rotation>");
    const auto begin = b.find("<Shape"), end = b.find("</Shape>", begin);
    b.replace(begin, end + 8 - begin, shape);
    std::string xml = "<Simulation xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance' "
                      "version='4.1'><World><Gravity x='0' y='-10'/><Bodies>" +
                      b + "</Bodies><BodyControllers>" + controllers +
                      "</BodyControllers><ScriptManager><Script><![CDATA[" + script +
                      "]]></Script></ScriptManager><GuiManager><GuiXML><![CDATA[" + gui +
                      "]]></GuiXML></GuiManager></World></Simulation>";
    mz_zip_archive zip{};
    REQUIRE(mz_zip_writer_init_heap(&zip, 0, 0));
    REQUIRE(
        mz_zip_writer_add_mem(&zip, "simulation.xml", xml.data(), xml.size(), MZ_BEST_COMPRESSION));
    void *data = nullptr;
    std::size_t count = 0;
    REQUIRE(mz_zip_writer_finalize_heap_archive(&zip, &data, &count));
    std::vector<std::uint8_t> bytes(static_cast<std::uint8_t *>(data),
                                    static_cast<std::uint8_t *>(data) + count);
    mz_free(data);
    mz_zip_writer_end(&zip);
    auto p = compat::inspect_ssim(bytes);
    REQUIRE(p.value());
    return *p.value();
}
const std::string rectangle =
    R"(<Shape xsi:type="Rectangle"><LocalCenter x="0" y="0"/><Width>2</Width><Height>1</Height><LocalRotation>0</LocalRotation></Shape>)";
} // namespace
TEST_CASE("General rectangle, nested sliders, force/friction controllers and reset callback",
          "[compatibility][rigid]") {
    auto p = rigid_project(
        rectangle,
        R"(<BodyController type="ForceController" enabled="true" bodyid="a"><xExpr>a</xExpr><yExpr>0</yExpr><ExtForcePoint x="0" y="1"/><ForceMode>2</ForceMode></BodyController><BodyController type="ValuePropertiesController" enabled="null;null;null;null;b;null;null;null;null;null;null;null;null;null;null;" bodyid="a"/>)",
        "function Reset_onClick(){let "
        "b=World.getBody('ball');b.reset();b.setPosition(2,3);b.setRotation(.4);}",
        R"(<desktop><dialog><textarea text="Independent synthetic friction controls"/><slider name="a" maximum="20" value="11" text="F=value"/><slider name="b" maximum="1" value=".7" text="mu=value"/><button name="Reset" text="Reset"/></dialog></desktop>)");
    auto result = compat::MechanicalSource::create(p);
    REQUIRE(result.value());
    auto source = *result.value();
    REQUIRE(source.definition().bodies[0].fixtures[0].vertices.size() == 4);
    REQUIRE(source.widgets().size() == 4);
    scene::MechanismSnapshot state;
    state.bodies.push_back({{1}, {}, {}, 1.57, 0});
    auto controls = source.controls(state);
    REQUIRE(controls.value());
    REQUIRE(controls.value()->forces[0].force == math::Vec2{11, 0});
    REQUIRE(controls.value()->forces[0].wrapped);
    REQUIRE(controls.value()->forces[0].torque_arm == -1);
    REQUIRE(controls.value()->friction.at({1}) == .7);
    REQUIRE(source.set_slider(2, .2).has_value());
    REQUIRE(source.controls(state).value()->friction.at({1}) == .2);
    REQUIRE(source.set_slider(2, 2).error());
    REQUIRE(source.apply_action(source.widgets()[3].action).has_value());
    REQUIRE(source.definition().bodies[0].center == math::Vec2{2, 3});
    REQUIRE(source.definition().bodies[0].angle == .4);
}
TEST_CASE("Concave source polygons and unknown required controllers retain diagnostics",
          "[compatibility][rigid]") {
    REQUIRE(
        compat::MechanicalSource::create(
            rigid_project(
                R"(<Shape xsi:type="Polygon"><LocalCenter x="0" y="0"/><Vertex x="0" y="0"/><Vertex x="1" y="0"/><Vertex x=".2" y=".2"/><Vertex x="0" y="1"/></Shape>)"))
            .error());
    REQUIRE(compat::MechanicalSource::create(
                rigid_project(rectangle, R"(<BodyController type="UnknownForce" bodyid="a"/>)"))
                .error());
    REQUIRE(
        compat::MechanicalSource::create(
            rigid_project(
                rectangle,
                R"(<BodyController type="ValuePropertiesController" enabled="1;null;null;null;null;" bodyid="a"/>)"))
            .error());
}

TEST_CASE("Source origin, local COM, fixture pose and script COM positions are distinct",
          "[compatibility][rigid]") {
    auto result = compat::MechanicalSource::create(rigid_project(rectangle, "", "", "", .5, 90));
    REQUIRE(result.value());
    auto source = *result.value();
    const auto &b = source.definition().bodies[0];
    REQUIRE(std::abs(b.center.x) < 1e-12);
    REQUIRE(std::abs(b.center.y - .5) < 1e-12);
    REQUIRE(b.fixtures[0].vertices[0] == math::Vec2{-1.5, -.5});
    REQUIRE(source.apply_action("World.getBody('ball').setPosition(2,3)").has_value());
    REQUIRE(source.definition().bodies[0].center == math::Vec2{2, 3});
    REQUIRE(source.apply_action("World.getBody('ball').setRotation(.4)").has_value());
    REQUIRE(source.definition().bodies[0].center == math::Vec2{2, 3});
}
