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

/* The lens's shaders. Lengths (amount, radius, amplitude) are in points; `radius`-like fractions are of the shader
 * radius, half the lens's diagonal. */

/** @brief Magnifies the middle, bends the content towards the rim and mirrors a narrow edge. */
struct RefractionShader {
    float refraction = 1.2f;
    float edge_reflection = 0.8f;
};

/** @brief Pulls red and blue apart along the radius, at the edge only or everywhere. */
struct ChromaticAberrationShader {
    float amount = 12.0f;
    float falloff = 1.8f;
    bool edge_only = true;
};

/** @brief A frosted blur, wider towards the rim by `edge_bias`. */
struct FrostedBlurShader {
    float radius = 8.0f;
    float intensity = 0.72f;
    float edge_bias = 0.35f;
};

/** @brief Enlarges the middle by `scale`, easing back to 1 at the rim. */
struct MagnificationShader {
    float scale = 1.55f;
    float falloff = 1.25f;
    float center_radius = 0.42f;
};

/** @brief Concentric waves pushing the content along the radius. */
struct RippleShader {
    float amplitude = 9.0f;
    float frequency = 9.0f;
    float phase = 0.0f;
    float falloff = 1.1f;
};

/** @brief Turns the content around the centre by up to `angle` radians, most in the middle. */
struct TwirlShader {
    float angle = 2.2f;
    float falloff = 1.15f;
    float center_radius = 0.18f;
};

/** @brief Warm streaks of focused light. */
struct CausticsShader {
    float intensity = 0.32f;
    float scale = 8.0f;
    float falloff = 1.25f;
};

/** @brief Darkens towards the rim, with a little contrast. */
struct VignetteShader {
    float intensity = 0.55f;
    float radius = 0.38f;
    float softness = 0.42f;
};

/** @brief A warm to cool glow at the visible edge. */
struct RimGlowShader {
    float intensity = 0.72f;
    float width = 0.16f;
    float softness = 0.2f;
};

/** @brief Small star glints scattered near the rim. */
struct SparkleShader {
    float intensity = 0.42f;
    float scale = 34.0f;
    float threshold = 0.78f;
};

/** @brief Splits the colours along facet directions. */
struct PrismShader {
    float amount = 10.0f;
    float facets = 8.0f;
    float falloff = 1.1f;
};

/** @brief Tints the content towards a glass colour. */
struct ColorGlassShader {
    float red = 0.45f;
    float green = 0.78f;
    float blue = 1.0f;
    float alpha = 0.28f;
};

/** @brief A grain of the given colour, one cell a point. */
struct NoiseShader {
    float red = 1.0f;
    float green = 1.0f;
    float blue = 1.0f;
    float alpha = 0.22f;
    float density = 0.36f;
};

/** @brief Rainbow bands of a thin film. */
struct SoapBubbleShader {
    float intensity = 0.55f;
    float scale = 9.0f;
    float phase = 0.0f;
};

/** @brief One step of a lens's shader chain; each samples what the one before produced. */
using LensShader = std::variant<RefractionShader, ChromaticAberrationShader, FrostedBlurShader, MagnificationShader,
                                RippleShader, TwirlShader, CausticsShader, VignetteShader, RimGlowShader, SparkleShader,
                                PrismShader, ColorGlassShader, NoiseShader, SoapBubbleShader>;

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
