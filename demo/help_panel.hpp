#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <string>
#include <vector>

namespace demo {

/**
 * @brief A list of the demo's keys and what is on, drawn into the content (so the lens shows it changing too). Not
 * in the reference demo: there the controls row does this; here it helps to test.
 */
class HelpPanel {
public:
    enum class Kind { kHeader, kOn, kOff, kNote };
    struct Line {
        std::string text;
        Kind kind = Kind::kNote;
    };

    HelpPanel() = default;
    ~HelpPanel();

    HelpPanel(const HelpPanel&) = delete;
    HelpPanel& operator=(const HelpPanel&) = delete;

    bool open(SDL_Renderer* renderer);
    void close();

    /** @brief Draws `lines` in a dark box with its top left at `x`, `y` (points), `width` wide. */
    void draw(float x, float y, float width, const std::vector<Line>& lines);

private:
    SDL_Renderer* renderer_ = nullptr;
    TTF_TextEngine* engine_ = nullptr;
    TTF_Font* font_ = nullptr;
    TTF_Font* bold_ = nullptr;
    std::vector<TTF_Text*> texts_;  // kept between frames: only changed lines are laid out again
};

} // namespace demo
