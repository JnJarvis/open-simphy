#include <algorithm>
#include <cmath>
#include <limits>
#include <opensim/scene/scene.hpp>
#include <type_traits>

namespace opensim::scene {
namespace {
core::Diagnostic error(core::Code code, std::string path, std::optional<core::EntityId> id = {}) {
    return {code, core::Severity::error, "Invalid scene field: " + path, id, std::move(path)};
}
bool positive(double value) { return std::isfinite(value) && value > 0; }
std::optional<core::Diagnostic> vector_error(math::Vec2 value, const std::string &path,
                                             std::optional<core::EntityId> id = {}) {
    if (!std::isfinite(value.x))
        return error(core::Code::invalid_data, path + ".x", id);
    if (!std::isfinite(value.y))
        return error(core::Code::invalid_data, path + ".y", id);
    return {};
}
std::optional<core::Diagnostic> fields_error(const ParticleFields &fields, core::EntityId id) {
    if (!positive(fields.mass))
        return error(core::Code::invalid_data, "particles.mass", id);
    if (!positive(fields.display_radius))
        return error(core::Code::invalid_data, "particles.display_radius", id);
    if (auto e = vector_error(fields.initial_position, "particles.position", id))
        return e;
    return vector_error(fields.initial_velocity, "particles.velocity", id);
}
core::Result<Edit> edited(const Document &original, std::vector<Particle> particles,
                          math::Vec2 gravity, core::EntityId mark,
                          std::optional<core::EntityId> affected) {
    auto document = Document::create(original.revision(), gravity, std::move(particles), mark);
    if (document.error())
        return core::Result<Edit>::failure(*document.error());
    return core::Result<Edit>::success({*document.value(), affected});
}
std::optional<core::Diagnostic> reference_error(const Document &document, core::EntityId id) {
    if (!id.valid())
        return error(core::Code::invalid_argument, "particles.id", id);
    if (std::none_of(document.particles().begin(), document.particles().end(),
                     [id](const Particle &p) { return p.id == id; }))
        return error(core::Code::missing_reference, "particles.id", id);
    return {};
}
} // namespace
Document::Document(math::Vec2 gravity, std::vector<Particle> particles, core::EntityId mark)
    : gravity_(gravity), particles_(std::move(particles)), high_water_(mark) {}
core::Result<Document> Document::create(unsigned revision, math::Vec2 gravity,
                                        std::vector<Particle> particles, core::EntityId mark) {
    if (revision != 1)
        return core::Result<Document>::failure(
            error(core::Code::unsupported_feature, "schema_revision"));
    if (auto e = vector_error(gravity, "gravity"))
        return core::Result<Document>::failure(*e);
    std::sort(particles.begin(), particles.end(),
              [](const Particle &a, const Particle &b) { return a.id < b.id; });
    core::EntityId previous{};
    for (const auto &p : particles) {
        if (!p.id.valid())
            return core::Result<Document>::failure(
                error(core::Code::invalid_data, "particles.id", p.id));
        if (p.id == previous)
            return core::Result<Document>::failure(
                error(core::Code::duplicate_id, "particles.id", p.id));
        if (auto e = fields_error(p.fields, p.id))
            return core::Result<Document>::failure(*e);
        previous = p.id;
    }
    if (mark < previous)
        return core::Result<Document>::failure(
            error(core::Code::invalid_data, "high_water", previous));
    return core::Result<Document>::success(Document(gravity, std::move(particles), mark));
}
core::Result<Edit> add_particle(const Document &document, ParticleFields fields) {
    if (document.high_water().value == std::numeric_limits<std::uint64_t>::max())
        return core::Result<Edit>::failure(error(core::Code::id_exhausted, "high_water"));
    const core::EntityId id{document.high_water().value + 1};
    if (auto e = fields_error(fields, id))
        return core::Result<Edit>::failure(*e);
    std::vector<Particle> particles(document.particles().begin(), document.particles().end());
    particles.push_back({id, fields});
    return edited(document, std::move(particles), document.gravity(), id, id);
}
core::Result<Edit> replace_particle(const Document &document, core::EntityId id,
                                    ParticleFields fields) {
    if (auto e = reference_error(document, id))
        return core::Result<Edit>::failure(*e);
    if (auto e = fields_error(fields, id))
        return core::Result<Edit>::failure(*e);
    std::vector<Particle> particles(document.particles().begin(), document.particles().end());
    for (auto &p : particles)
        if (p.id == id)
            p.fields = fields;
    return edited(document, std::move(particles), document.gravity(), document.high_water(), id);
}
core::Result<Edit> remove_particle(const Document &document, core::EntityId id) {
    if (auto e = reference_error(document, id))
        return core::Result<Edit>::failure(*e);
    std::vector<Particle> particles(document.particles().begin(), document.particles().end());
    std::erase_if(particles, [id](const Particle &p) { return p.id == id; });
    return edited(document, std::move(particles), document.gravity(), document.high_water(), id);
}
core::Result<Edit> set_gravity(const Document &document, math::Vec2 gravity) {
    return edited(document, {document.particles().begin(), document.particles().end()}, gravity,
                  document.high_water(), {});
}
Snapshot::Snapshot(double time, math::Vec2 gravity, std::vector<ParticleSample> particles)
    : time_(time), gravity_(gravity), particles_(std::move(particles)) {}
core::Result<Snapshot> Snapshot::create(const Document &document, double time,
                                        std::vector<ParticleSample> particles) {
    if (!std::isfinite(time) || time < 0)
        return core::Result<Snapshot>::failure(error(core::Code::invalid_data, "time"));
    std::sort(particles.begin(), particles.end(),
              [](const auto &a, const auto &b) { return a.id < b.id; });
    core::EntityId previous{};
    for (const auto &p : particles) {
        if (!p.id.valid())
            return core::Result<Snapshot>::failure(
                error(core::Code::invalid_data, "particles.id", p.id));
        if (p.id == previous)
            return core::Result<Snapshot>::failure(
                error(core::Code::duplicate_id, "particles.id", p.id));
        if (auto e = fields_error({p.mass, p.position, p.velocity, p.display_radius}, p.id))
            return core::Result<Snapshot>::failure(*e);
        previous = p.id;
    }
    if (particles.size() != document.particles().size())
        return core::Result<Snapshot>::failure(error(core::Code::invalid_data, "particles"));
    for (std::size_t i = 0; i < particles.size(); ++i) {
        const auto &p = particles[i];
        const auto &initial = document.particles()[i];
        if (p.id != initial.id)
            return core::Result<Snapshot>::failure(
                error(core::Code::missing_reference, "particles.id", p.id));
        if (p.mass != initial.fields.mass)
            return core::Result<Snapshot>::failure(
                error(core::Code::invalid_data, "particles.mass", p.id));
        if (p.display_radius != initial.fields.display_radius)
            return core::Result<Snapshot>::failure(
                error(core::Code::invalid_data, "particles.display_radius", p.id));
    }
    return core::Result<Snapshot>::success(
        Snapshot(time, document.gravity(), std::move(particles)));
}
core::Result<Snapshot> Snapshot::initial(const Document &document) {
    std::vector<ParticleSample> samples;
    samples.reserve(document.particles().size());
    for (const auto &p : document.particles())
        samples.push_back({p.id, p.fields.mass, p.fields.initial_position,
                           p.fields.initial_velocity, p.fields.display_radius});
    return create(document, 0, std::move(samples));
}
DrawPacket::DrawPacket(std::vector<Primitive> primitives) : primitives_(std::move(primitives)) {}
core::Result<DrawPacket> DrawPacket::create(std::vector<Primitive> primitives) {
    for (const auto &primitive : primitives) {
        const bool valid = std::visit(
            [](const auto &p) {
                for (double channel : {p.color.r, p.color.g, p.color.b, p.color.a})
                    if (!std::isfinite(channel) || channel < 0 || channel > 1)
                        return false;
                if constexpr (std::is_same_v<std::decay_t<decltype(p)>, Circle>)
                    return math::finite(p.center) && positive(p.radius);
                else
                    return math::finite(p.from) && math::finite(p.to) && positive(p.width);
            },
            primitive);
        if (!valid)
            return core::Result<DrawPacket>::failure(error(core::Code::invalid_data, "primitives"));
    }
    return core::Result<DrawPacket>::success(DrawPacket(std::move(primitives)));
}
} // namespace opensim::scene
