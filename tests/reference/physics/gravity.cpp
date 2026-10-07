#include "numeric.hpp"
#include <opensim/physics/world.hpp>
using namespace opensim;
TEST_CASE("P06-P08 trajectories match independent analytic reference") {
    struct Vector {
        double h;
        unsigned n;
        double vy, g, y_final, vy_final, tolerance, time_tolerance;
    };
    // SI units. Analytic values from accepted PHY-001, not captured implementation output.
    for (const auto vector : {Vector{1.0 / 128, 128, 3, -8, 9, -5, 1e-11, 1e-15},
                              Vector{0.01, 100, 2, -9.8, 7.1, -7.8, 1e-11, 1e-15},
                              Vector{0.01, 1000, 2, -9.8, -460, -96, 1e-8, 1e-14}}) {
        CAPTURE(vector.h, vector.n);
        const auto document = scene::Document::create(
            1, {0, vector.g}, {{{1}, {1, {0, 10}, {2, vector.vy}, 0.1}}}, {1});
        REQUIRE(document.value());
        auto created = physics::World::create(*document.value(), vector.h);
        REQUIRE(created.value());
        auto world = *created.value();
        for (unsigned i = 0; i < vector.n; ++i) {
            auto next = world.step();
            REQUIRE(next.value());
            world = *next.value();
        }
        const auto snapshot = world.snapshot();
        const double t = vector.n * vector.h;
        CHECK(world.step_index() == vector.n);
        OPENSIM_CHECK_NEAR(snapshot.time(), t, vector.time_tolerance, 0);
        OPENSIM_CHECK_NEAR(snapshot.particles()[0].position.x, 2 * t, vector.tolerance, 0);
        OPENSIM_CHECK_NEAR(snapshot.particles()[0].position.y, vector.y_final, vector.tolerance, 0);
        OPENSIM_CHECK_NEAR(snapshot.particles()[0].velocity.x, 2, vector.tolerance, 0);
        OPENSIM_CHECK_NEAR(snapshot.particles()[0].velocity.y, vector.vy_final, vector.tolerance,
                           0);
    }
}
