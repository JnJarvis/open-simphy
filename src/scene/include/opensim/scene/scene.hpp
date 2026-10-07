#pragma once
#include <opensim/core/values.hpp>
#include <opensim/math/math.hpp>
#include <span>
#include <variant>
#include <vector>

namespace opensim::scene {
struct ParticleFields {
    double mass;
    math::Vec2 initial_position, initial_velocity;
    double display_radius;
    bool operator==(const ParticleFields &) const = default;
};
struct Particle {
    core::EntityId id;
    ParticleFields fields;
    bool operator==(const Particle &) const = default;
};
class Document {
    math::Vec2 gravity_;
    std::vector<Particle> particles_;
    core::EntityId high_water_;
    Document(math::Vec2 gravity, std::vector<Particle> particles, core::EntityId high_water);

  public:
    [[nodiscard]] static core::Result<Document> create(unsigned revision, math::Vec2 gravity,
                                                       std::vector<Particle> particles,
                                                       core::EntityId high_water);
    [[nodiscard]] unsigned revision() const { return 1; }
    [[nodiscard]] math::Vec2 gravity() const { return gravity_; }
    [[nodiscard]] core::EntityId high_water() const { return high_water_; }
    [[nodiscard]] std::span<const Particle> particles() const { return particles_; }
    bool operator==(const Document &) const = default;
};
struct Edit {
    Document document;
    std::optional<core::EntityId> affected_id;
};
[[nodiscard]] core::Result<Edit> add_particle(const Document &, ParticleFields);
[[nodiscard]] core::Result<Edit> replace_particle(const Document &, core::EntityId, ParticleFields);
[[nodiscard]] core::Result<Edit> remove_particle(const Document &, core::EntityId);
[[nodiscard]] core::Result<Edit> set_gravity(const Document &, math::Vec2);

struct ParticleSample {
    core::EntityId id;
    double mass;
    math::Vec2 position, velocity;
    double display_radius;
    bool operator==(const ParticleSample &) const = default;
};
class Snapshot {
    double time_;
    math::Vec2 gravity_;
    std::vector<ParticleSample> particles_;
    Snapshot(double time, math::Vec2 gravity, std::vector<ParticleSample> particles);

  public:
    // The starting document fixes IDs, mass, radius and gravity for the run.
    [[nodiscard]] static core::Result<Snapshot>
    create(const Document &starting_document, double time, std::vector<ParticleSample> particles);
    [[nodiscard]] static core::Result<Snapshot> initial(const Document &);
    [[nodiscard]] double time() const { return time_; }
    [[nodiscard]] math::Vec2 gravity() const { return gravity_; }
    [[nodiscard]] std::span<const ParticleSample> particles() const { return particles_; }
    bool operator==(const Snapshot &) const = default;
};
struct Color {
    double r, g, b, a;
    bool operator==(const Color &) const = default;
};
struct Circle {
    math::Vec2 center;
    double radius;
    Color color;
};
struct Line {
    math::Vec2 from, to;
    double width;
    Color color;
};
using Primitive = std::variant<Circle, Line>;
class DrawPacket {
    std::vector<Primitive> primitives_;
    explicit DrawPacket(std::vector<Primitive> primitives);

  public:
    [[nodiscard]] static core::Result<DrawPacket> create(std::vector<Primitive> primitives);
    [[nodiscard]] std::span<const Primitive> primitives() const { return primitives_; }
};
} // namespace opensim::scene
