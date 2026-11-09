#include "demo_background.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace demo {
namespace {

/* The system's colours of the reference demo: purple, pink, orange, yellow, top left to bottom right. */
constexpr std::array<SDL_FColor, 4> kGradient{
    SDL_FColor{175 / 255.0f, 82 / 255.0f, 222 / 255.0f, 1.0f},
    SDL_FColor{255 / 255.0f, 45 / 255.0f, 85 / 255.0f, 1.0f},
    SDL_FColor{255 / 255.0f, 149 / 255.0f, 0 / 255.0f, 1.0f},
    SDL_FColor{255 / 255.0f, 204 / 255.0f, 0 / 255.0f, 1.0f},
};
constexpr int kGrid = 48;
constexpr Uint64 kCounterStepMs = 50;

constexpr std::array<const char*, 3> kBoldFonts{
    "/usr/share/fonts/truetype/noto/NotoSans-Bold.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
    "/System/Library/Fonts/Supplemental/Arial Rounded Bold.ttf",
};

TTF_Font* open_font(float size)
{
    for (const char* path : kBoldFonts) {
        if (TTF_Font* font = TTF_OpenFont(path, size)) {
            return font;
        }
    }
    return nullptr;
}

/** @brief The gradient's colour at `t` from 0 (top left) to 1 (bottom right), its four stops evenly spaced. */
SDL_FColor gradient_at(float t)
{
    const float scaled = std::clamp(t, 0.0f, 1.0f) * 3.0f;
    const auto index = std::min(2, static_cast<int>(scaled));
    const float u = scaled - static_cast<float>(index);
    const SDL_FColor& a = kGradient[static_cast<std::size_t>(index)];
    const SDL_FColor& b = kGradient[static_cast<std::size_t>(index) + 1];
    return SDL_FColor{a.r + (b.r - a.r) * u, a.g + (b.g - a.g) * u, a.b + (b.b - a.b) * u, 1.0f};
}

} // namespace

DemoBackground::~DemoBackground()
{
    close();
}

bool DemoBackground::open(SDL_Renderer* renderer)
{
    renderer_ = renderer;
    if (!TTF_WasInit() && !TTF_Init()) {
        return false;
    }
    counter_font_ = open_font(60.0f);
    title_font_ = open_font(38.0f);
    body_font_ = open_font(18.0f);
    return counter_font_ != nullptr && title_font_ != nullptr && body_font_ != nullptr;
}

void DemoBackground::close()
{
    for (TTF_Font** font : {&counter_font_, &title_font_, &body_font_}) {
        if (*font != nullptr) {
            TTF_CloseFont(*font);
            *font = nullptr;
        }
    }
}

void DemoBackground::update(Uint64 now_ms)
{
    if (next_step_ == 0) {
        next_step_ = now_ms + kCounterStepMs;
    }
    while (now_ms >= next_step_) {
        int next = counter_ + direction_;
        if (next > 100) {
            next = 100;
            direction_ = -1;
        } else if (next < 0) {
            next = 0;
            direction_ = 1;
        }
        counter_ = next;
        next_step_ += kCounterStepMs;
    }
}

void DemoBackground::draw_text(TTF_Font* font, const char* text, SDL_Color colour, float centre_x, float top,
                               float alpha, float shadow_radius, SDL_Color shadow, float shadow_y)
{
    SDL_Surface* face = TTF_RenderText_Blended_Wrapped(font, text, 0, colour, 0);
    if (face == nullptr) {
        return;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, face);
    const auto w = static_cast<float>(face->w);
    const auto h = static_cast<float>(face->h);
    SDL_DestroySurface(face);
    if (texture == nullptr) {
        return;
    }
    const SDL_FRect at{std::round(centre_x - w / 2.0f), std::round(top), w, h};
    // A soft shadow: the text in the shadow's colour, spread around in a ring and faint.
    if (shadow_radius > 0.0f) {
        SDL_SetTextureColorMod(texture, shadow.r, shadow.g, shadow.b);
        const int rings = 12;
        SDL_SetTextureAlphaMod(texture, static_cast<Uint8>(std::min(255.0f, shadow.a / static_cast<float>(rings) * 2.2f)));
        for (int i = 0; i < rings; ++i) {
            const float angle = static_cast<float>(i) / rings * 6.2831853f;
            const SDL_FRect blur{at.x + std::cos(angle) * shadow_radius * 0.5f,
                                 at.y + shadow_y + std::sin(angle) * shadow_radius * 0.5f, w, h};
            SDL_RenderTexture(renderer_, texture, nullptr, &blur);
        }
        SDL_SetTextureColorMod(texture, 255, 255, 255);
    }
    SDL_SetTextureAlphaMod(texture, static_cast<Uint8>(255.0f * alpha));
    SDL_RenderTexture(renderer_, texture, nullptr, &at);
    SDL_DestroyTexture(texture);
}

void DemoBackground::draw(float width, float height)
{
    // The gradient on a grid: its colour is linear along the diagonal of the unit square, so a fine mesh is exact
    // but for the bends at the stops.
    std::array<SDL_Vertex, (kGrid + 1) * (kGrid + 1)> vertices{};
    for (int y = 0; y <= kGrid; ++y) {
        for (int x = 0; x <= kGrid; ++x) {
            const float u = static_cast<float>(x) / kGrid;
            const float v = static_cast<float>(y) / kGrid;
            vertices[static_cast<std::size_t>(y * (kGrid + 1) + x)] =
                SDL_Vertex{{u * width, v * height}, gradient_at((u + v) / 2.0f), {0.0f, 0.0f}};
        }
    }
    std::array<int, kGrid * kGrid * 6> indices{};
    std::size_t n = 0;
    for (int y = 0; y < kGrid; ++y) {
        for (int x = 0; x < kGrid; ++x) {
            const int a = y * (kGrid + 1) + x;
            for (const int index : {a, a + 1, a + kGrid + 2, a, a + kGrid + 2, a + kGrid + 1}) {
                indices[n++] = index;
            }
        }
    }
    SDL_RenderGeometry(renderer_, nullptr, vertices.data(), static_cast<int>(vertices.size()), indices.data(),
                       static_cast<int>(indices.size()));

    // The counter, the title and the two lines, one column with 24 between them, in the middle of the page.
    const std::string counter = std::to_string(counter_);
    int counter_h = 0;
    int title_h = 0;
    int body_h = 0;
    TTF_GetStringSize(counter_font_, counter.c_str(), 0, nullptr, &counter_h);
    TTF_GetStringSize(title_font_, "Glass Lens Demo", 0, nullptr, &title_h);
    const char* body = "Drag the small circle.\nThe lens refracts and slightly reflects edges.";
    TTF_GetStringSizeWrapped(body_font_, body, 0, 0, nullptr, &body_h);
    TTF_SetFontWrapAlignment(body_font_, TTF_HORIZONTAL_ALIGN_CENTER);
    const float column = static_cast<float>(counter_h + title_h + body_h) + 48.0f;
    float top = (height - column) / 2.0f;
    const float centre = width / 2.0f;
    const SDL_Color shadow{0, 0, 0, 85};
    draw_text(counter_font_, counter.c_str(), SDL_Color{0, 0, 0, 255}, centre, top, 1.0f, 12.0f, shadow, 0.0f);
    draw_text(counter_font_, counter.c_str(), SDL_Color{0, 0, 0, 255}, centre, top, 1.0f, 4.0f,
              SDL_Color{255, 255, 255, 77}, 2.0f);
    top += static_cast<float>(counter_h) + 24.0f;
    draw_text(title_font_, "Glass Lens Demo", SDL_Color{255, 255, 255, 255}, centre, top, 1.0f, 12.0f, shadow, 0.0f);
    top += static_cast<float>(title_h) + 24.0f;
    draw_text(body_font_, body, SDL_Color{255, 255, 255, 255}, centre, top, 0.9f, 12.0f, shadow, 0.0f);
}

} // namespace demo
