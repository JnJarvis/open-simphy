#pragma once
#include <compare>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace opensim::core {
struct EntityId {
    std::uint64_t value{};
    [[nodiscard]] constexpr bool valid() const { return value != 0; }
    auto operator<=>(const EntityId &) const = default;
};
enum class Code {
    invalid_argument,
    id_exhausted,
    duplicate_id,
    missing_reference,
    unsupported_feature,
    invalid_data,
    internal_error
};
enum class Severity { info, warning, error };
[[nodiscard]] constexpr std::string_view code_name(Code code) {
    switch (code) {
    case Code::invalid_argument:
        return "invalid_argument";
    case Code::id_exhausted:
        return "id_exhausted";
    case Code::duplicate_id:
        return "duplicate_id";
    case Code::missing_reference:
        return "missing_reference";
    case Code::unsupported_feature:
        return "unsupported_feature";
    case Code::invalid_data:
        return "invalid_data";
    case Code::internal_error:
        return "internal_error";
    }
    return "unknown";
}
struct Diagnostic {
    Code code;
    Severity severity;
    std::string message;
    std::optional<EntityId> entity;
    std::optional<std::string> path;
    bool operator==(const Diagnostic &) const = default;
};
inline Diagnostic checked_error(Diagnostic error) {
    if (error.severity != Severity::error) {
        return {Code::invalid_argument, Severity::error, "Failure requires error severity",
                std::nullopt, std::nullopt};
    }
    return error;
}
template <class T> class [[nodiscard]] Result {
    std::variant<T, Diagnostic> state_;
    template <std::size_t I, class V>
    Result(std::in_place_index_t<I> index, V &&value) : state_(index, std::forward<V>(value)) {}

  public:
    [[nodiscard]] static Result success(T value) {
        return Result(std::in_place_index<0>, std::move(value));
    }
    [[nodiscard]] static Result failure(Diagnostic error) {
        return Result(std::in_place_index<1>, checked_error(std::move(error)));
    }
    // Immutable branch access prevents invalidating the result discriminant.
    [[nodiscard]] const T *value() const { return std::get_if<0>(&state_); }
    [[nodiscard]] const Diagnostic *error() const { return std::get_if<1>(&state_); }
    [[nodiscard]] bool has_value() const { return state_.index() == 0; }
    Result(const Result &) = default;
    Result(Result &&) = default;
    Result &operator=(const Result &) = delete;
    Result &operator=(Result &&) = delete;
};
template <> class [[nodiscard]] Result<void> {
    std::optional<Diagnostic> error_;
    explicit Result(std::optional<Diagnostic> error) : error_(std::move(error)) {}

  public:
    [[nodiscard]] static Result success() { return Result(std::nullopt); }
    [[nodiscard]] static Result failure(Diagnostic error) {
        return Result(checked_error(std::move(error)));
    }
    [[nodiscard]] bool has_value() const { return !error_; }
    [[nodiscard]] const Diagnostic *error() const { return error_ ? &*error_ : nullptr; }
};
class IdAllocator {
    std::uint64_t high_water_{};

  public:
    [[nodiscard]] EntityId high_water() const { return {high_water_}; }
    [[nodiscard]] Result<EntityId> next() {
        if (high_water_ == std::numeric_limits<std::uint64_t>::max()) {
            return Result<EntityId>::failure({Code::id_exhausted, Severity::error,
                                              "Entity IDs exhausted", std::nullopt, std::nullopt});
        }
        return Result<EntityId>::success({++high_water_});
    }
    [[nodiscard]] Result<void> reserve_through(EntityId id) {
        if (!id.valid())
            return Result<void>::failure({Code::invalid_argument, Severity::error,
                                          "Cannot reserve invalid ID", id, std::nullopt});
        if (id.value > high_water_)
            high_water_ = id.value;
        return Result<void>::success();
    }
};
} // namespace opensim::core
