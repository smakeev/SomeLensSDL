#include "demo_background.hpp"

#include "somelens/shader_program.hpp"
#include "generated/passthrough.msl.h"
#include "generated/passthrough.spv.h"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <cstdio>
#include <cstdlib>

/* Stage 1 of the demo: the page under the lens, drawn into a texture and shown through a custom fragment shader on
 * SDL's GPU renderer, which is the path every lens takes. The window title shows the frame time. */
int main(int /*argc*/, char** /*argv*/)
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }
    SDL_Window* window = SDL_CreateWindow("SomeLens SDL", 1000, 700, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    SDL_Renderer* renderer = window != nullptr ? somelens::create_gpu_renderer(window) : nullptr;
    if (renderer == nullptr) {
        std::fprintf(stderr, "no GPU renderer: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }
    SDL_SetRenderVSync(renderer, 1);
    SDL_Log("renderer: %s, GPU driver: %s", SDL_GetRendererName(renderer),
            SDL_GetGPUDeviceDriver(SDL_GetGPURendererDevice(renderer)));

    somelens::ShaderProgram passthrough;
    if (!passthrough.create(renderer, {passthrough_spv, sizeof passthrough_spv, passthrough_msl, sizeof passthrough_msl, 0})) {
        std::fprintf(stderr, "shader failed: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }
    demo::DemoBackground background;
    if (!background.open(renderer)) {
        std::fprintf(stderr, "fonts failed: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    SDL_Texture* content = nullptr;
    int content_w = 0;
    int content_h = 0;
    double frame_ms = 0.0;
    Uint64 last = SDL_GetTicksNS();
    Uint64 title_at = 0;
    const char* snapshot_path = SDL_getenv("SOMELENS_DEMO_SNAPSHOT");
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT
                || (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)) {
                running = false;
            }
        }
        int w = 0;
        int h = 0;
        SDL_GetCurrentRenderOutputSize(renderer, &w, &h);
        if (content == nullptr || w != content_w || h != content_h) {
            if (content != nullptr) {
                SDL_DestroyTexture(content);
            }
            content = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);
            content_w = w;
            content_h = h;
        }
        background.update(SDL_GetTicks());

        // The content into its texture, then that texture to the window through the custom shader.
        SDL_SetRenderTarget(renderer, content);
        background.draw(static_cast<float>(w), static_cast<float>(h));
        SDL_SetRenderTarget(renderer, nullptr);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_SetGPURenderState(renderer, passthrough.state());
        SDL_RenderTexture(renderer, content, nullptr, nullptr);
        SDL_SetGPURenderState(renderer, nullptr);
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
    if (content != nullptr) {
        SDL_DestroyTexture(content);
    }
    background.close();
    passthrough.destroy();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return EXIT_SUCCESS;
}
