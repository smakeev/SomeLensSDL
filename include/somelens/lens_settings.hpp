#pragma once

#include <SDL3/SDL.h>

#include <functional>
#include <optional>
#include <variant>
#include <vector>

namespace somelens {

/**
 * @brief A lens outline: given the lens rectangle, the closed outline as points, in order. Curves are given already
 * flattened; the lens fills the outline and strokes its ring along it.
 */
using LensPath = std::function<std::vector<SDL_FPoint>(const SDL_FRect& rect)>;

/** @brief An ellipse filling the rectangle: a circle when it is square. */
[[nodiscard]] LensPath ellipse_path();

struct RefractionShader {
    float refraction = 1.2f;
    float edge_reflection = 0.8f;
};

/** @brief One step of a lens's shader chain; each samples what the one before produced. */
using LensShader = std::variant<RefractionShader>;

struct LensSettings {
    float width = 160.0f;
    float height = 160.0f;
    LensPath path = ellipse_path();
    float ring_width = 1.0f;
    bool animates_path_changes = false;
    std::vector<LensShader> shaders{RefractionShader{}};

    [[nodiscard]] static LensSettings circle(float diameter = 160.0f);
};

/** @brief How often a container takes a new snapshot of its content for its lenses. */
struct SnapshotRefreshRate {
    enum class Kind { kNever, kAutomatic, kFast, kBalanced, kRelaxed, kSlow, kCustom };
    Kind kind = Kind::kAutomatic;
    int milliseconds = 200;  // for kCustom

    /** @brief Seconds between snapshots; empty for kNever. */
    [[nodiscard]] std::optional<double> interval() const;
    bool operator==(const SnapshotRefreshRate&) const = default;
};

} // namespace somelens
