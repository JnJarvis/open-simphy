#include <algorithm>
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
                              double angle = 0, const std::string &joints = "",
                              const std::string &fields = "") {
    std::string b = body;
    const auto mass_center = b.find("<LocalCenter");
    const auto mass_end = b.find("/>", mass_center);
    b.replace(mass_center, mass_end + 2 - mass_center,
              "<LocalCenter x=\"" + std::to_string(com) + "\" y=\"0\"/>");
    b.replace(b.find("<Rotation>0</Rotation>"), 22,
              "<Rotation>" + std::to_string(angle) + "</Rotation>");
    const auto begin = b.find("<Shape"), end = b.find("</Shape>", begin);
    b.replace(begin, end + 8 - begin, shape);
    std::string xml =
        "<Simulation xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance' "
        "version='4.1'><World><Gravity x='0' y='-10'/><Bodies>" +
        b +
        "<Body "
        "Id=\"ground\"><Mass><Type>INFINITE</Type></Mass><Fixtures/></Body></Bodies><Joints>" +
        joints + "</Joints><BodyControllers>" + controllers +
        "</BodyControllers><ScriptManager><Script><![CDATA[" + script +
        "]]></Script></ScriptManager><GuiManager><GuiXML><![CDATA[" + gui +
        "]]></GuiXML></GuiManager></World>" + fields + "</Simulation>";
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
TEST_CASE("Source line axis and anchors preserve reference frames without disabled infinity limits",
          "[compatibility][INT-010]") {
    const std::string line =
        R"(<Joint xsi:type="LineJoint"><BodyId1>a</BodyId1><BodyId2>a</BodyId2><LocalAnchor x="3" y="4"/><LineAngle>1.5707963267948966</LineAngle><LimitEnabled>false</LimitEnabled><LowerLimit>-Infinity</LowerLimit><UpperLimit>Infinity</UpperLimit></Joint>)";
    // Replace B with a separately defined body by making A the fixtureless ground.
    auto grounded = line;
    grounded.replace(grounded.find("<BodyId1>a"), 10, "<BodyId1>ground");
    auto result =
        compat::MechanicalSource::create(rigid_project(rectangle, "", "", "", 0, 0, grounded));
    REQUIRE(result.value());
    auto s = result.value()->definition().slides.at(0);
    REQUIRE_FALSE(s.body_a.valid());
    REQUIRE(s.body_b == core::EntityId{1});
    REQUIRE(s.local_a == math::Vec2{3, 4});
    REQUIRE(std::abs(s.axis.x) < 1e-12);
    REQUIRE(s.axis.y == 1);
    REQUIRE_FALSE(s.limit);
    auto enabled = grounded;
    enabled.replace(enabled.find("<LimitEnabled>false"), 19, "<LimitEnabled>true");
    REQUIRE(compat::MechanicalSource::create(rigid_project(rectangle, "", "", "", 0, 0, enabled))
                .error());
    REQUIRE(
        compat::MechanicalSource::create(rigid_project(rectangle, "", "", "", 0, 0, line)).error());
}
TEST_CASE("Required top level force fields cannot be silently omitted",
          "[compatibility][INT-010]") {
    auto r = compat::MechanicalSource::create(rigid_project(
        rectangle, "", "", "", 0, 0, "",
        R"(<Fields><Field xsi:type="GravitationalField" enabled="true"><xExprForce>10</xExprForce><yExprForce>0</yExprForce></Field></Fields>)"));
    REQUIRE(r.error());
    REQUIRE(r.error()->message.find("fields") != std::string::npos);
}
TEST_CASE("Hidden slider bindings survive unusable display bounds and inherit visibility",
          "[compatibility][INT-010]") {
    auto p = rigid_project(
        rectangle,
        R"(<BodyController type="ForceController" enabled="true" bodyid="a"><xExpr>a+w</xExpr><yExpr>0</yExpr><ExtForcePoint x="0" y="0"/><ForceMode>0</ForceMode></BodyController>)",
        "",
        R"(<desktop><slider name="w" value="5.8"/><panel visible="false"><slider name="a" value="0" rectbounds="1060,145,-132,33"/></panel><slider name="disabled" enabled="false"/></desktop>)");
    auto r = compat::MechanicalSource::create(p);
    REQUIRE(r.value());
    auto s = *r.value();
    REQUIRE(s.widgets().size() == 3);
    REQUIRE(s.widgets()[0].maximum == 10);
    REQUIRE(s.widgets()[0].value == 5.8);
    REQUIRE_FALSE(s.widgets()[1].visible);
    REQUIRE(s.widgets()[1].size.x == -132);
    REQUIRE_FALSE(s.widgets()[2].enabled);
    REQUIRE(s.widgets()[2].value == 5);
    scene::MechanismSnapshot state;
    state.bodies.push_back({{1}, {}, {}, 0, 0});
    auto c = s.controls(state);
    REQUIRE(c.value());
    REQUIRE(c.value()->forces.at(0).force.x == 5.8);
    REQUIRE(s.set_slider(1, 2).has_value());
    auto changed = s.controls(state);
    REQUIRE(changed.value());
    REQUIRE(changed.value()->forces.at(0).force.x == 7.8);
    REQUIRE(s.set_slider(2, 3).error());
}
TEST_CASE("Source clock requests survive startup preflight and are consumed once",
          "[compatibility][INT-010]") {
    auto p = rigid_project(
        rectangle,
        R"(<BodyController type="ForceController" enabled="true" bodyid="a"><xExpr>World.getSimulationTime()</xExpr><yExpr>T</yExpr><ExtForcePoint x="0" y="0"/><ForceMode>0</ForceMode></BodyController>)",
        "World.setSimulationTime(7);function reset(){World.setSimulationTime(0);}");
    auto r = compat::MechanicalSource::create(p);
    REQUIRE(r.value());
    auto s = *r.value();
    scene::MechanismSnapshot state;
    state.bodies.push_back({{1}, {}, {}, 0, 0});
    auto first = s.controls(state);
    REQUIRE(first.value());
    REQUIRE(first.value()->time == 7);
    REQUIRE(first.value()->forces.at(0).force == math::Vec2{7, 7});
    state.time = 7.5;
    auto next = s.controls(state);
    REQUIRE(next.value());
    REQUIRE_FALSE(next.value()->time);
    REQUIRE(next.value()->forces.at(0).force == math::Vec2{7.5, 7.5});
    REQUIRE(s.apply_action("reset()").has_value());
    auto reset = s.controls(state);
    REQUIRE(reset.value());
    REQUIRE(reset.value()->time == 0);
    REQUIRE(reset.value()->forces.at(0).force == math::Vec2{});
    REQUIRE(s.apply_action("World.setSimulationTime(-1)").error());
    REQUIRE(compat::MechanicalSource::create(project("World.setSimulationTime(Infinity)")).error());
}
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
TEST_CASE("Simple concave polygons preserve area and unknown controllers retain diagnostics",
          "[compatibility][rigid]") {
    auto result = compat::MechanicalSource::create(rigid_project(
        R"(<Shape xsi:type="Polygon"><LocalCenter x="0" y="0"/><Vertex x="0" y="0"/><Vertex x="1" y="0"/><Vertex x=".2" y=".2"/><Vertex x="0" y="1"/></Shape>)"));
    REQUIRE(result.value());
    REQUIRE(result.value()->definition().bodies[0].fixtures.size() == 2);
    double area = 0;
    for (const auto &f : result.value()->definition().bodies[0].fixtures)
        for (std::size_t i = 0; i < f.vertices.size(); ++i) {
            const auto a = f.vertices[i], b = f.vertices[(i + 1) % f.vertices.size()];
            area += (a.x * b.y - a.y * b.x) / 2;
        }
    REQUIRE(std::abs(area - .2) < 1e-12); // Hull would incorrectly occupy .5.
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

TEST_CASE("Polygon pieces retain notches, clockwise outlines and reject crossing boundaries",
          "[compatibility][rigid]") {
    const auto shape = [](const std::vector<math::Vec2> &v) {
        std::string out = "<Shape xsi:type='Polygon'><LocalCenter x='0' y='0'/>";
        for (auto q : v)
            out += "<Vertex x='" + std::to_string(q.x) + "' y='" + std::to_string(q.y) + "'/>";
        return out + "</Shape>";
    };
    std::vector<math::Vec2> outline{{0, 0}, {3, 0}, {3, 1}, {1, 1}, {1, 3}, {0, 3}};
    const auto coverage = [](const scene::CircleBody &shape_body, math::Vec2 p) {
        unsigned count = 0;
        for (const auto &f : shape_body.fixtures) {
            bool in = true;
            for (std::size_t i = 0; i < f.vertices.size(); ++i) {
                auto a = f.vertices[i], b = f.vertices[(i + 1) % f.vertices.size()];
                in = in && (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x) >= -1e-10;
            }
            count += in ? 1 : 0;
        }
        return count;
    };
    for (int order = 0; order < 2; ++order) {
        auto result = compat::MechanicalSource::create(rigid_project(shape(outline)));
        REQUIRE(result.value());
        const auto &b = result.value()->definition().bodies[0];
        REQUIRE(b.fixtures.size() == 4);
        // Interior points deliberately avoid shared triangle boundaries.
        REQUIRE(coverage(b, {.23, 2.51}) == 1);
        REQUIRE(coverage(b, {2.51, .23}) == 1);
        REQUIRE(coverage(b, {2, 2}) == 0);
        double area = 0;
        for (const auto &f : b.fixtures) {
            REQUIRE(f.friction == .3);
            REQUIRE(f.restitution == 1);
            for (std::size_t i = 0; i < 3; ++i) {
                auto a = f.vertices[i], z = f.vertices[(i + 1) % 3];
                area += (a.x * z.y - a.y * z.x) / 2;
            }
        }
        REQUIRE(std::abs(area - 5) < 1e-12);
        std::reverse(outline.begin(), outline.end());
    }
    REQUIRE(compat::MechanicalSource::create(rigid_project(shape({{0, 0}, {2, 2}, {0, 2}, {2, 0}})))
                .error());
    REQUIRE(compat::MechanicalSource::create(rigid_project(shape({{0, 0}, {2, 0}, {1, 0}, {1, 1}})))
                .error());
    REQUIRE(compat::MechanicalSource::create(rigid_project(shape(std::vector<math::Vec2>(65))))
                .error());
    std::vector<math::Vec2> many;
    for (int i = 0; i < 12; ++i)
        many.push_back(
            {std::cos(i * 6.283185307179586 / 12), std::sin(i * 6.283185307179586 / 12)});
    auto large = compat::MechanicalSource::create(rigid_project(shape(many)));
    REQUIRE(large.value());
    REQUIRE(large.value()->definition().bodies[0].fixtures.size() == 10);
}

TEST_CASE("Authored elastic distance rope and weld parameters survive source reset",
          "[compatibility][rigid]") {
    const std::string common =
        "<BodyId1>a</BodyId1><BodyId2>ground</BodyId2><CollisionAllowed>true</CollisionAllowed>";
    const std::string anchors = "<Anchor1 x='0' y='0'/><Anchor2 x='2' y='0'/>";
    const auto source = [&](const std::string &joint) {
        auto result =
            compat::MechanicalSource::create(rigid_project(rectangle, "", "", "", 0, 0, joint));
        REQUIRE(result.value());
        return *result.value();
    };
    auto spring = source("<Joint xsi:type='SpringJoint'>" + common + anchors +
                         "<SpringConstant>8</SpringConstant><DampingRatio>.6</"
                         "DampingRatio><Frequency>7</Frequency><distance>2</distance></Joint>");
    REQUIRE(spring.definition().links[0].stiffness == 8);
    REQUIRE(spring.definition().links[0].damping_coefficient == .6);
    REQUIRE(spring.definition().links[0].collide_connected);
    REQUIRE(spring.apply_action("World.getBody('ball').reset()").has_value());
    REQUIRE(spring.definition().links[0].stiffness == 8);
    auto distance = source(
        "<Joint xsi:type='DistanceJoint'>" + common + anchors +
        "<Frequency>2</Frequency><DampingRatio>.5</DampingRatio><Distance>2</Distance></Joint>");
    REQUIRE(std::abs(distance.definition().links[0].stiffness - 157.913670417) < 1e-6);
    REQUIRE(distance.definition().links[0].damping_ratio == .5);
    auto rope =
        source("<Joint xsi:type='RopeJoint'>" + common + anchors +
               "<LowerLimitEnabled>true</LowerLimitEnabled><UpperLimitEnabled>true</"
               "UpperLimitEnabled><LowerLimit>0</LowerLimit><UpperLimit>3</UpperLimit></Joint>");
    REQUIRE(rope.definition().links[0].limit);
    REQUIRE(rope.definition().links[0].minimum == 0);
    REQUIRE(rope.definition().links[0].maximum == 3);
    auto weld = source("<Joint xsi:type='WeldJoint'>" + common +
                       "<Anchor x='0' "
                       "y='0'/><ReferenceAngle>.3</ReferenceAngle><Frequency>2</"
                       "Frequency><DampingRatio>.7</DampingRatio></Joint>");
    REQUIRE(weld.definition().welds.size() == 1);
    REQUIRE(weld.definition().welds[0].reference == -.3);
    REQUIRE(weld.definition().welds[0].frequency == 2);
}
