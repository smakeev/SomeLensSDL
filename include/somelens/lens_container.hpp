#pragma once

#include "somelens/lens_settings.hpp"

#include <SDL3/SDL.h>

namespace somelens {

/**
 * @brief Holds the content lenses look at: the caller draws its content into the container each frame, the
 * container shows it and, at its refresh rate, keeps a snapshot of it for the lenses (copied on the GPU).
 *
 *     container.begin_content();   // draw the content now, in points
 *     ...
 *     container.end_content();     // shows it, and snapshots it when due
 *     lens.draw(container, centre, settings);
 */
class LensContainer {
public:
    LensContainer() = default;
    ~LensContainer();

    LensContainer(const LensContainer&) = delete;
    LensContainer& operator=(const LensContainer&) = delete;

    void open(SDL_Renderer* renderer);
    void close();

    void set_refresh_rate(const SnapshotRefreshRate& rate);
    [[nodiscard]] const SnapshotRefreshRate& refresh_rate() const noexcept { return rate_; }
    /** @brief While paused no snapshot is taken; one is taken as soon as it resumes. */
    void set_paused(bool paused);
    /** @brief Asks for a snapshot as soon as the rate allows, e.g. after the content changed. */
    void request_snapshot();

    /** @brief Starts the frame's content: the render target becomes the container, the window's size, in points. */
    void begin_content();
    /**
     * @brief Ends it: draws the content to the window (unless `present` is false, for a caller that shows the content
     * itself) and takes the snapshot when due.
     */
    void end_content(bool present = true);

    [[nodiscard]] SDL_Renderer* renderer() const noexcept { return renderer_; }
    /** @brief The latest snapshot, or null before the first one. */
    [[nodiscard]] SDL_Texture* snapshot() const noexcept { return has_snapshot_ ? snapshot_ : nullptr; }
    /** @brief Pixels per point of the window. */
    [[nodiscard]] float pixel_scale() const noexcept { return scale_; }
    /** @brief How many snapshots were taken, to tell a new one. */
    [[nodiscard]] Uint64 snapshot_version() const noexcept { return version_; }

private:
    void take_snapshot();

    SDL_Renderer* renderer_ = nullptr;
    SDL_Texture* content_ = nullptr;
    SDL_Texture* snapshot_ = nullptr;
    int width_ = 0;
    int height_ = 0;
    float scale_ = 1.0f;
    SnapshotRefreshRate rate_{};
    bool paused_ = false;
    bool requested_ = true;
    bool has_snapshot_ = false;
    Uint64 version_ = 0;
    Uint64 last_finished_ns_ = 0;
    Uint64 last_duration_ns_ = 0;
};

} // namespace somelens
