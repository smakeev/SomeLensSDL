#include "demo_background.hpp"

#include "somelens/glass_lens.hpp"
#include "somelens/lens_container.hpp"
#include "somelens/shader_program.hpp"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>

namespace {

struct PathOption {
    const char* title;
    float width;
    float height;
    somelens::LensPath path;
};

/* The reference demo's outlines: Bezier curves of a blob for the custom one. */
somelens::LensPath custom_path()
{
    return [](const SDL_FRect& rect) {
        const float x = rect.x;
        const float y = rect.y;
        const float w = rect.w;
        const float h = rect.h;
        auto at = [&](float u, float v) { return SDL_FPoint{x + w * u, y + h * v}; };
        somelens::PathBuilder path;
        path.move_to(at(0.50f, 0.02f));
        path.cubic_to(at(0.72f, -0.06f), at(0.92f, 0.06f), at(0.93f, 0.25f));
        path.cubic_to(at(1.05f, 0.42f), at(0.94f, 0.61f), at(0.82f, 0.72f));
        path.cubic_to(at(0.70f, 0.87f), at(0.62f, 1.00f), at(0.48f, 0.96f));
        path.cubic_to(at(0.33f, 0.91f), at(0.18f, 0.95f), at(0.13f, 0.77f));
        path.cubic_to(at(-0.03f, 0.61f), at(0.02f, 0.39f), at(0.11f, 0.28f));
        path.cubic_to(at(0.19f, 0.10f), at(0.33f, 0.14f), at(0.50f, 0.02f));
        return path.points();
    };
}

somelens::LensPath diamond_path()
{
    return [](const SDL_FRect& rect) {
        somelens::PathBuilder path;
        path.move_to(SDL_FPoint{rect.x + rect.w / 2.0f, rect.y});
        path.line_to(SDL_FPoint{rect.x + rect.w, rect.y + rect.h / 2.0f});
        path.line_to(SDL_FPoint{rect.x + rect.w / 2.0f, rect.y + rect.h});
        path.line_to(SDL_FPoint{rect.x, rect.y + rect.h / 2.0f});
        return path.points();
    };
}

std::array<PathOption, 8> path_options()
{
    return {
        PathOption{"Circle", 160.0f, 160.0f, somelens::ellipse_path()},
        PathOption{"Ellipse", 190.0f, 130.0f, somelens::ellipse_path()},
        PathOption{"Square", 150.0f, 150.0f, somelens::rectangle_path()},
        PathOption{"Rectangle", 200.0f, 130.0f, somelens::rectangle_path()},
        PathOption{"Rounded", 180.0f, 140.0f, somelens::rounded_rectangle_path(0.24f)},
        PathOption{"Capsule", 210.0f, 110.0f, somelens::rounded_rectangle_path(0.5f)},
        PathOption{"Diamond", 160.0f, 160.0f, diamond_path()},
        PathOption{"Custom(Path)", 190.0f, 170.0f, custom_path()},
    };
}

/* Where the lens centre may go: the whole lens inside the window, 6 points of margin at the bottom. The lens keeps
 * its place as a ratio of these bounds, so it stays put relative to the window when the window is resized. */
SDL_FRect movement_bounds(float width, float height, const somelens::LensSettings& lens)
{
    const float min_x = lens.width / 2.0f;
    const float max_x = std::max(min_x, width - lens.width / 2.0f);
    const float min_y = lens.height / 2.0f;
    const float visual_margin = 6.0f;
    const float max_y = std::max(min_y, height - visual_margin - lens.height / 2.0f);
    return SDL_FRect{min_x, min_y, std::max(max_x - min_x, 1.0f), std::max(max_y - min_y, 1.0f)};
}

SDL_FPoint centre_at(const SDL_FRect& bounds, SDL_FPoint ratio)
{
    return SDL_FPoint{bounds.x + ratio.x * bounds.w, bounds.y + ratio.y * bounds.h};
}

SDL_FPoint ratio_of(const SDL_FRect& bounds, SDL_FPoint centre)
{
    const float x = std::clamp(centre.x, bounds.x, bounds.x + bounds.w);
    const float y = std::clamp(centre.y, bounds.y, bounds.y + bounds.h);
    return SDL_FPoint{(x - bounds.x) / bounds.w, (y - bounds.y) / bounds.h};
}

const char* rate_title(const somelens::SnapshotRefreshRate& rate, char* buffer, std::size_t size)
{
    using Kind = somelens::SnapshotRefreshRate::Kind;
    switch (rate.kind) {
    case Kind::kNever:
        return "Never";
    case Kind::kAutomatic:
        return "Automatic";
    case Kind::kFast:
        return "Fast";
    case Kind::kBalanced:
        return "Balanced";
    case Kind::kRelaxed:
        return "Relaxed";
    case Kind::kSlow:
        return "Slow";
    case Kind::kCustom:
        std::snprintf(buffer, size, "%d ms", rate.milliseconds);
        return buffer;
    }
    return "";
}

/* Until the controls row exists: keys 0-6 pick the rate (Never, Automatic, Fast, Balanced, Relaxed, Slow, custom),
 * Up and Down change the custom milliseconds by 10, P switches Pause move; Tab and Shift+Tab pick the outline,
 * A switches Animate. */
bool handle_key(SDL_Keycode key, somelens::SnapshotRefreshRate& rate, int& custom_ms, bool& pause_move)
{
    using Kind = somelens::SnapshotRefreshRate::Kind;
    switch (key) {
    case SDLK_0:
        rate = {Kind::kNever, custom_ms};
        return true;
    case SDLK_1:
        rate = {Kind::kAutomatic, custom_ms};
        return true;
    case SDLK_2:
        rate = {Kind::kFast, custom_ms};
        return true;
    case SDLK_3:
        rate = {Kind::kBalanced, custom_ms};
        return true;
    case SDLK_4:
        rate = {Kind::kRelaxed, custom_ms};
        return true;
    case SDLK_5:
        rate = {Kind::kSlow, custom_ms};
        return true;
    case SDLK_6:
        rate = {Kind::kCustom, custom_ms};
        return true;
    case SDLK_UP:
    case SDLK_DOWN:
        custom_ms = std::clamp(custom_ms + (key == SDLK_UP ? 10 : -10), 10, 5000);
        rate = {Kind::kCustom, custom_ms};
        return true;
    case SDLK_P:
        pause_move = !pause_move;
        return true;
    default:
        return false;
    }
}

} // namespace

/* The demo: the page under a draggable glass lens. The window title shows the frame time. */
int main(int /*argc*/, char** /*argv*/)
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }
    SDL_Window* window =
        SDL_CreateWindow("SomeLens SDL", 1000, 700, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    SDL_Renderer* renderer = window != nullptr ? somelens::create_gpu_renderer(window) : nullptr;
    if (renderer == nullptr) {
        std::fprintf(stderr, "no GPU renderer: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }
    SDL_SetRenderVSync(renderer, 1);
    SDL_Log("renderer: %s, GPU driver: %s", SDL_GetRendererName(renderer),
            SDL_GetGPUDeviceDriver(SDL_GetGPURendererDevice(renderer)));

    demo::DemoBackground background;
    if (!background.open(renderer)) {
        std::fprintf(stderr, "fonts failed: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    somelens::LensContainer container;
    container.open(renderer);
    somelens::GlassLens lens;
    const std::array<PathOption, 8> paths = path_options();
    std::size_t path_index = 0;
    bool animate = true;
    somelens::LensSettings lens_settings = somelens::LensSettings::circle(160.0f);
    SDL_FPoint lens_ratio{0.5f, 0.5f};
    bool dragging = false;
    somelens::SnapshotRefreshRate rate{};
    int custom_ms = 200;
    bool pause_move = false;
    SDL_FPoint grab{0.0f, 0.0f};  // the pointer's offset from the lens centre while dragging
    double frame_ms = 0.0;
    Uint64 last = SDL_GetTicksNS();
    Uint64 title_at = 0;
    const char* snapshot_path = SDL_getenv("SOMELENS_DEMO_SNAPSHOT");
    bool running = true;
    while (running) {
        SDL_Event event;
        lens_settings.width = paths[path_index].width;
        lens_settings.height = paths[path_index].height;
        lens_settings.path = paths[path_index].path;
        lens_settings.animates_path_changes = animate;
        int window_w = 0;
        int window_h = 0;
        SDL_GetWindowSize(window, &window_w, &window_h);
        const SDL_FRect bounds =
            movement_bounds(static_cast<float>(window_w), static_cast<float>(window_h), lens_settings);
        SDL_FPoint lens_centre = centre_at(bounds, lens_ratio);
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT
                || (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)) {
                running = false;
            } else if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_A) {
                animate = !animate;
                title_at = 0;
            } else if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_TAB) {
                const bool back = (event.key.mod & SDL_KMOD_SHIFT) != 0;
                path_index = (path_index + (back ? paths.size() - 1 : 1)) % paths.size();
                title_at = 0;
            } else if (event.type == SDL_EVENT_KEY_DOWN) {
                if (handle_key(event.key.key, rate, custom_ms, pause_move)) {
                    title_at = 0;
                }
            } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
                const SDL_FPoint at{event.button.x, event.button.y};
                if (container.snapshot() != nullptr && lens.contains(lens_centre, lens_settings, at)) {
                    dragging = true;
                    grab = SDL_FPoint{at.x - lens_centre.x, at.y - lens_centre.y};
                }
            } else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT) {
                dragging = false;
            } else if (event.type == SDL_EVENT_MOUSE_MOTION && dragging) {
                lens_ratio = ratio_of(bounds, SDL_FPoint{event.motion.x - grab.x, event.motion.y - grab.y});
                lens_centre = centre_at(bounds, lens_ratio);
            }
        }
        background.update(SDL_GetTicks());

        // The page into the container (it shows it and snapshots it at its rate), then the lens over it.
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        container.set_refresh_rate(rate);
        container.set_paused(dragging && pause_move);
        container.begin_content();
        background.draw(static_cast<float>(window_w), static_cast<float>(window_h));
        container.end_content();
        lens.draw(container, lens_centre, lens_settings);
        // SOMELENS_DEMO_SNAPSHOT=/path.png saves one frame a second after start, to look at without a screen.
        if (snapshot_path != nullptr && SDL_GetTicks() > 1000) {
            if (SDL_Surface* shot = SDL_RenderReadPixels(renderer, nullptr)) {
                SDL_SavePNG(shot, snapshot_path);
                SDL_DestroySurface(shot);
            }
            snapshot_path = nullptr;
        }
        SDL_RenderPresent(renderer);

        const Uint64 now = SDL_GetTicksNS();
        frame_ms = frame_ms * 0.95 + static_cast<double>(now - last) / 1e6 * 0.05;
        last = now;
        if (now - title_at > 500'000'000) {
            char custom[32];
            char title[160];
            std::snprintf(title, sizeof title, "SomeLens SDL - %s - Animate %s - %s - Pause move %s - %.1f ms a frame",
                          paths[path_index].title, animate ? "on" : "off", rate_title(rate, custom, sizeof custom),
                          pause_move ? "on" : "off", frame_ms);
            SDL_SetWindowTitle(window, title);
            title_at = now;
        }
    }
    lens.release();
    container.close();
    background.close();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return EXIT_SUCCESS;
}
