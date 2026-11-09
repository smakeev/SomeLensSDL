# SomeLensSDL

Glass lens effects for SDL3. A lens lies over any content drawn with SDL's renderer and shows the content under it
through a chain of GPU shaders: refraction, chromatic aberration, frosted blur, magnification, ripple, twirl,
caustics, vignette, rim glow, sparkle, prism, colour glass, noise and soap bubble. A lens takes any outline, can move
and change shape smoothly, and refreshes its view of the content at a chosen rate.

Built on SDL 3.4's GPU renderer with custom fragment shaders written in GLSL and shipped precompiled as SPIR-V
(Vulkan) and MSL (Metal).

```sh
make install_tools   # cmake, SDL3, SDL3_ttf and the shader tools
make run             # build and start the demo
make shaders         # after changing shaders/*.frag: regenerate src/generated/
```

## Use

```cpp
#include "somelens/glass_lens.hpp"

SDL_Renderer* renderer = somelens::create_gpu_renderer(window);
somelens::LensContainer container;
container.open(renderer);
container.set_refresh_rate({somelens::SnapshotRefreshRate::Kind::kFast});

somelens::GlassLens lens;
somelens::LensSettings settings = somelens::LensSettings::circle(160.0f);
settings.shaders = {somelens::RefractionShader{}, somelens::RimGlowShader{}};
settings.animates_path_changes = true;

// Every frame:
container.begin_content();   // draw the content here, in points
draw_my_content();
container.end_content();     // shows it and snapshots it when due
lens.draw(container, SDL_FPoint{x, y}, settings);
SDL_RenderPresent(renderer);
```

An outline is any closed shape: `ellipse_path()`, `rectangle_path()`, `rounded_rectangle_path(fraction)`, or a
function building one with `PathBuilder` (lines and cubic curves). The demo in `demo/` shows every outline, shader
and refresh rate.

MIT licence, see `LICENSE`.
