#pragma once

#include "somelens/lens_settings.hpp"

#include <SDL3/SDL.h>

#include <memory>
#include <vector>

namespace somelens {

/**
 * @brief Turns the settings a lens is given into the settings it shows: when they change and ask for it
 * (animates_path_changes), the outline, size, ring and shader values spring from what is shown to the new ones.
 * Outlines are compared and blended as 96 points spread evenly along them, in the unit square.
 */
class LensMorph {
public:
    /** @brief The settings to draw at `now_ns`, given the ones asked for. */
    [[nodiscard]] LensSettings shown(const LensSettings& target, Uint64 now_ns);
    [[nodiscard]] bool animating() const noexcept { return animating_; }

private:
    struct Shape {
        float width = 0.0f;
        float height = 0.0f;
        float ring_width = 0.0f;
        std::shared_ptr<const std::vector<SDL_FPoint>> outline;  // unit square
        std::vector<LensShader> shaders;
    };

    struct Signature {
        int width = 0;
        int height = 0;
        int ring_width = 0;
        bool animates = false;
        std::vector<int> shaders;
        std::vector<int> outline;
        bool operator==(const Signature&) const = default;
    };

    [[nodiscard]] static Shape shape_of(const LensSettings& settings);
    [[nodiscard]] static Signature signature_of(const LensSettings& settings);
    [[nodiscard]] static Shape blend(const Shape& from, const Shape& to, float progress);
    [[nodiscard]] Shape current(Uint64 now_ns) const;
    [[nodiscard]] static LensSettings settings_of(const Shape& shape, const LensSettings& target);

    bool has_shown_ = false;
    bool animating_ = false;
    Signature signature_;
    Shape from_;
    Shape to_;
    Uint64 start_ns_ = 0;
};

} // namespace somelens
