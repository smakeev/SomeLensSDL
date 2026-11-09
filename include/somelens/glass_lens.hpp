#pragma once

#include "somelens/lens_container.hpp"
#include "somelens/lens_settings.hpp"
#include "somelens/shader_program.hpp"

#include <SDL3/SDL.h>

#include <memory>
#include <vector>

namespace somelens {

/**
 * @brief A lens of glass over a container: the part of the container's snapshot under it, run through its chain of
 * shaders, cut to its outline, with a ring along the outline. Keep one per lens; it holds the lens's GPU textures.
 */
class GlassLens {
public:
    GlassLens();
    ~GlassLens();

    GlassLens(const GlassLens&) = delete;
    GlassLens& operator=(const GlassLens&) = delete;

    /**
     * @brief Draws the lens centred at `centre` (points, in the container) over what is on screen. When the settings
     * change and ask for it (animates_path_changes), the lens springs to them over about half a second; keep drawing
     * every frame meanwhile.
     */
    void draw(LensContainer& container, SDL_FPoint centre, const LensSettings& settings);

    /** @brief Whether a point (points, in the container) lies inside the lens's outline, for picking it up. */
    [[nodiscard]] bool contains(SDL_FPoint centre, const LensSettings& settings, SDL_FPoint point) const;

    void release();

private:
    void render(LensContainer& container, SDL_FPoint centre, const LensSettings& settings);

    struct Resources;
    std::unique_ptr<Resources> r_;
};

} // namespace somelens
