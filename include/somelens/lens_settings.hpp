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

/**
 * @brief Builds an outline from lines and curves, flattened into points: what a LensPath returns.
 *
 *     PathBuilder path;
 *     path.move_to({x, y});
 *     path.cubic_to(control1, control2, {x2, y2});
 *     return path.points();
 */
class PathBuilder {
public:
    void move_to(SDL_FPoint p);
    void line_to(SDL_FPoint p);
    void cubic_to(SDL_FPoint control1, SDL_FPoint control2, SDL_FPoint p);
    /** @brief A circular arc around `centre` from `start` to `end` radians, clockwise on screen when end > start. */
    void arc(SDL_FPoint centre, float radius, float start, float end);
    void add_ellipse(const SDL_FRect& rect);
    void add_rectangle(const SDL_FRect& rect);
    /** @brief A rectangle with circular corners of `radius`, clamped to half the shorter side. */
    void add_rounded_rectangle(const SDL_FRect& rect, float radius);

    /** @brief The outline, closed implicitly, without a repeated last point. */
    [[nodiscard]] std::vector<SDL_FPoint> points() const;

private:
    void add(SDL_FPoint p);
    std::vector<SDL_FPoint> points_;
};

/** @brief An ellipse filling the rectangle: a circle when it is square. */
[[nodiscard]] LensPath ellipse_path();
/** @brief The rectangle itself. */
[[nodiscard]] LensPath rectangle_path();
/** @brief The rectangle with circular corners, their radius `fraction` of its shorter side. */
[[nodiscard]] LensPath rounded_rectangle_path(float fraction);

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
