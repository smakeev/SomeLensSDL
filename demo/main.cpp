#include "demo_background.hpp"

#include "somelens/glass_lens.hpp"
#include "somelens/lens_container.hpp"
#include "somelens/shader_program.hpp"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>

namespace {

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
    const somelens::LensSettings lens_settings = somelens::LensSettings::circle(160.0f);
    SDL_FPoint lens_ratio{0.5f, 0.5f};
    bool dragging = false;
    SDL_FPoint grab{0.0f, 0.0f};  // the pointer's offset from the lens centre while dragging
    double frame_ms = 0.0;
    Uint64 last = SDL_GetTicksNS();
    Uint64 title_at = 0;
    const char* snapshot_path = SDL_getenv("SOMELENS_DEMO_SNAPSHOT");
    bool running = true;
    while (running) {
        SDL_Event event;
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
            char title[96];
            std::snprintf(title, sizeof title, "SomeLens SDL - %.1f ms a frame", frame_ms);
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
