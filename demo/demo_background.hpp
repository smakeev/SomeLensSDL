#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

namespace demo {

/**
 * @brief The content under the lens: the gradient, the counter bouncing 0..100 and the two lines of text of the
 * reference demo, laid out the same way.
 */
class DemoBackground {
public:
    DemoBackground() = default;
    ~DemoBackground();

    DemoBackground(const DemoBackground&) = delete;
    DemoBackground& operator=(const DemoBackground&) = delete;

    bool open(SDL_Renderer* renderer);
    void close();

    /** @brief The counter steps once every 50 ms, up to 100 and back down to 0. */
    void update(Uint64 now_ms);
    [[nodiscard]] int counter() const noexcept { return counter_; }

    /** @brief Draws the whole background into the current render target, `width` by `height`. */
    void draw(float width, float height);

private:
    void draw_text(TTF_Font* font, const char* text, SDL_Color colour, float centre_x, float top, float alpha,
                   float shadow_radius, SDL_Color shadow, float shadow_y);

    SDL_Renderer* renderer_ = nullptr;
    TTF_Font* counter_font_ = nullptr;
    TTF_Font* title_font_ = nullptr;
    TTF_Font* body_font_ = nullptr;
    int counter_ = 0;
    int direction_ = 1;
    Uint64 next_step_ = 0;
};

} // namespace demo
