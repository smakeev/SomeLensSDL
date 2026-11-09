#include "somelens/lens_container.hpp"

#include <algorithm>

namespace somelens {

LensContainer::~LensContainer()
{
    close();
}

void LensContainer::open(SDL_Renderer* renderer)
{
    close();
    renderer_ = renderer;
    requested_ = true;
}

void LensContainer::close()
{
    for (SDL_Texture** texture : {&content_, &snapshot_}) {
        if (*texture != nullptr) {
            SDL_DestroyTexture(*texture);
            *texture = nullptr;
        }
    }
    has_snapshot_ = false;
    width_ = 0;
    height_ = 0;
    renderer_ = nullptr;
}

void LensContainer::set_refresh_rate(const SnapshotRefreshRate& rate)
{
    if (rate_ != rate) {
        rate_ = rate;
        requested_ = true;
    }
}

void LensContainer::set_paused(bool paused)
{
    if (paused_ && !paused) {
        requested_ = true;
    }
    paused_ = paused;
}

void LensContainer::request_snapshot()
{
    requested_ = true;
}

void LensContainer::begin_content()
{
    int w = 0;
    int h = 0;
    SDL_GetCurrentRenderOutputSize(renderer_, &w, &h);
    if (content_ == nullptr || w != width_ || h != height_) {
        for (SDL_Texture** texture : {&content_, &snapshot_}) {
            if (*texture != nullptr) {
                SDL_DestroyTexture(*texture);
            }
            *texture = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);
            SDL_SetTextureBlendMode(*texture, SDL_BLENDMODE_NONE);
        }
        width_ = w;
        height_ = h;
        has_snapshot_ = false;
        requested_ = true;
    }
    int window_w = 0;
    SDL_GetWindowSize(SDL_GetRenderWindow(renderer_), &window_w, nullptr);
    scale_ = window_w > 0 ? static_cast<float>(w) / static_cast<float>(window_w) : 1.0f;
    SDL_SetRenderTarget(renderer_, content_);
    SDL_SetRenderScale(renderer_, scale_, scale_);
}

void LensContainer::end_content(bool present)
{
    SDL_SetRenderScale(renderer_, 1.0f, 1.0f);
    SDL_SetRenderTarget(renderer_, nullptr);
    if (present) {
        SDL_RenderTexture(renderer_, content_, nullptr, nullptr);
    }

    // A snapshot is due at the rate, or when asked for (rate changed, resumed); either way never closer to the last
    // one than the rate's interval or one and a half times what the last capture took. With kNever only asked ones.
    const std::optional<double> interval = rate_.interval();
    const double effective =
        std::max(interval.value_or(0.0), static_cast<double>(last_duration_ns_) * 1.5 / 1e9);
    const double elapsed = static_cast<double>(SDL_GetTicksNS() - last_finished_ns_) / 1e9;
    const bool wanted = requested_ || interval.has_value();
    const bool due = !has_snapshot_ || (wanted && elapsed >= effective);
    if (due && !paused_) {
        take_snapshot();
    }
}

void LensContainer::take_snapshot()
{
    const Uint64 start = SDL_GetTicksNS();
    // A copy on the GPU: the content texture drawn into the snapshot texture, nothing goes through the CPU.
    SDL_SetRenderTarget(renderer_, snapshot_);
    SDL_RenderTexture(renderer_, content_, nullptr, nullptr);
    SDL_SetRenderTarget(renderer_, nullptr);
    has_snapshot_ = true;
    requested_ = false;
    ++version_;
    last_finished_ns_ = SDL_GetTicksNS();
    last_duration_ns_ = last_finished_ns_ - start;
}

} // namespace somelens
