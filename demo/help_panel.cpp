#include "help_panel.hpp"

#include <array>

namespace demo {
namespace {

constexpr float kFontSize = 14.0f;
constexpr float kLineHeight = 19.0f;
constexpr float kPadding = 10.0f;

constexpr std::array<const char*, 3> kRegularFonts{
    "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    "/System/Library/Fonts/Supplemental/Arial.ttf",
};
constexpr std::array<const char*, 3> kBoldFonts{
    "/usr/share/fonts/truetype/noto/NotoSans-Bold.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
    "/System/Library/Fonts/Supplemental/Arial Bold.ttf",
};

template <std::size_t N>
TTF_Font* open_first(const std::array<const char*, N>& paths)
{
    for (const char* path : paths) {
        if (TTF_Font* font = TTF_OpenFont(path, kFontSize)) {
            return font;
        }
    }
    return nullptr;
}

} // namespace

HelpPanel::~HelpPanel()
{
    close();
}

bool HelpPanel::open(SDL_Renderer* renderer)
{
    renderer_ = renderer;
    if (!TTF_WasInit() && !TTF_Init()) {
        return false;
    }
    engine_ = TTF_CreateRendererTextEngine(renderer);
    font_ = open_first(kRegularFonts);
    bold_ = open_first(kBoldFonts);
    return engine_ != nullptr && font_ != nullptr && bold_ != nullptr;
}

void HelpPanel::close()
{
    for (TTF_Text* text : texts_) {
        TTF_DestroyText(text);
    }
    texts_.clear();
    if (font_ != nullptr) {
        TTF_CloseFont(font_);
        font_ = nullptr;
    }
    if (bold_ != nullptr) {
        TTF_CloseFont(bold_);
        bold_ = nullptr;
    }
    if (engine_ != nullptr) {
        TTF_DestroyRendererTextEngine(engine_);
        engine_ = nullptr;
    }
}

void HelpPanel::draw(float x, float y, float width, const std::vector<Line>& lines)
{
    if (engine_ == nullptr) {
        return;
    }
    const SDL_FRect box{x, y, width, kPadding * 2.0f + kLineHeight * static_cast<float>(lines.size())};
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 90);
    SDL_RenderFillRect(renderer_, &box);

    while (texts_.size() < lines.size()) {
        texts_.push_back(TTF_CreateText(engine_, font_, "", 0));
    }
    for (std::size_t i = 0; i < lines.size(); ++i) {
        TTF_Text* text = texts_[i];
        if (text == nullptr) {
            continue;
        }
        const Line& line = lines[i];
        TTF_Font* font = line.kind == Kind::kHeader ? bold_ : font_;
        if (TTF_GetTextFont(text) != font) {
            TTF_SetTextFont(text, font);
        }
        if (line.text != (text->text != nullptr ? text->text : "")) {
            TTF_SetTextString(text, line.text.c_str(), 0);
        }
        switch (line.kind) {
        case Kind::kHeader:
            TTF_SetTextColor(text, 255, 255, 255, 255);
            break;
        case Kind::kOn:
            TTF_SetTextColor(text, 255, 255, 255, 255);
            break;
        case Kind::kOff:
            TTF_SetTextColor(text, 255, 255, 255, 150);
            break;
        case Kind::kNote:
            TTF_SetTextColor(text, 255, 255, 255, 190);
            break;
        }
        TTF_DrawRendererText(text, x + kPadding, y + kPadding + kLineHeight * static_cast<float>(i));
    }
}

} // namespace demo
