#pragma once
#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>

namespace opensim::math {
using Scalar = double;
static_assert(std::numeric_limits<Scalar>::is_iec559 && std::numeric_limits<Scalar>::digits == 53 &&
              std::numeric_limits<Scalar>::max_exponent == 1024);
struct Vec2 {
    Scalar x{}, y{};
    bool operator==(const Vec2 &) const = default;
};
[[nodiscard]] constexpr Vec2 operator+(Vec2 a, Vec2 b) { return {a.x + b.x, a.y + b.y}; }
[[nodiscard]] constexpr Vec2 operator-(Vec2 a, Vec2 b) { return {a.x - b.x, a.y - b.y}; }
[[nodiscard]] constexpr Vec2 operator-(Vec2 a) { return {-a.x, -a.y}; }
[[nodiscard]] constexpr Vec2 operator*(Vec2 a, Scalar b) { return {a.x * b, a.y * b}; }
[[nodiscard]] constexpr Vec2 operator*(Scalar a, Vec2 b) { return b * a; }
[[nodiscard]] constexpr Scalar dot(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }
[[nodiscard]] constexpr Scalar cross(Vec2 a, Vec2 b) { return a.x * b.y - a.y * b.x; }
[[nodiscard]] inline bool finite(Vec2 v) { return std::isfinite(v.x) && std::isfinite(v.y); }
[[nodiscard]] inline Scalar magnitude(Vec2 v) { return std::hypot(v.x, v.y); }
// No epsilon cutoff: finite tiny vectors are accepted when their norm is representable.
[[nodiscard]] inline std::optional<Vec2> normalized(Vec2 v) {
    if (!finite(v))
        return std::nullopt;
    const auto norm = magnitude(v);
    if (!std::isfinite(norm) || norm == 0)
        return std::nullopt;
    return Vec2{v.x / norm, v.y / norm};
}
[[nodiscard]] inline bool near(Scalar a, Scalar b, Scalar abs_tol, Scalar rel_tol) {
    if (!std::isfinite(a) || !std::isfinite(b) || !std::isfinite(abs_tol) ||
        !std::isfinite(rel_tol) || abs_tol < 0 || rel_tol < 0 || rel_tol > 1)
        return false;
    return std::abs(a - b) <= std::max(abs_tol, rel_tol * std::max(std::abs(a), std::abs(b)));
}
[[nodiscard]] inline bool near(Vec2 a, Vec2 b, Scalar abs_tol, Scalar rel_tol) {
    return near(a.x, b.x, abs_tol, rel_tol) && near(a.y, b.y, abs_tol, rel_tol);
}
class Transform2 {
    Vec2 translation_;
    Scalar angle_;
    Transform2(Vec2 translation, Scalar angle) : translation_(translation), angle_(angle) {}

  public:
    [[nodiscard]] static Transform2 identity() { return Transform2({0, 0}, 0); }
    [[nodiscard]] static std::optional<Transform2> create(Vec2 translation, Scalar angle) {
        if (!finite(translation) || !std::isfinite(angle))
            return std::nullopt;
        return Transform2(translation, angle);
    }
    [[nodiscard]] Vec2 translation() const { return translation_; }
    [[nodiscard]] Scalar angle() const { return angle_; }
    [[nodiscard]] std::optional<Vec2> direction(Vec2 v) const {
        if (!finite(v))
            return std::nullopt;
        const auto c = std::cos(angle_), s = std::sin(angle_);
        const Vec2 result{c * v.x - s * v.y, s * v.x + c * v.y};
        return finite(result) ? std::optional<Vec2>(result) : std::nullopt;
    }
    [[nodiscard]] std::optional<Vec2> point(Vec2 v) const {
        const auto rotated = direction(v);
        if (!rotated)
            return std::nullopt;
        const auto result = *rotated + translation_;
        return finite(result) ? std::optional<Vec2>(result) : std::nullopt;
    }
};
// compose(A,B) applies B first, then A. Checked arithmetic never publishes nonfinite state.
[[nodiscard]] inline std::optional<Transform2> compose(const Transform2 &a, const Transform2 &b) {
    const auto translation = a.point(b.translation());
    return translation ? Transform2::create(*translation, a.angle() + b.angle()) : std::nullopt;
}
[[nodiscard]] inline std::optional<Transform2> inverse(const Transform2 &value) {
    const auto rotation = Transform2::create({0, 0}, -value.angle());
    const auto translation = rotation->direction(-value.translation());
    return translation ? Transform2::create(*translation, -value.angle()) : std::nullopt;
}
} // namespace opensim::math
