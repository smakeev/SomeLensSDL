# SomeLensSDL

Glass lens effects for SDL3. A lens lies over any content drawn with SDL's renderer and shows the content under it
through a chain of GPU shaders: refraction, chromatic aberration, frosted blur, magnification, ripple, twirl,
caustics, vignette, rim glow, sparkle, prism, colour glass, noise and soap bubble. A lens takes any outline, can move
and change shape smoothly, and refreshes its view of the content at a chosen rate.

Built on SDL 3.4's GPU renderer (Vulkan, Metal, Direct3D 12) with custom fragment shaders written in GLSL and shipped
precompiled as SPIR-V and MSL.

```sh
make install_tools   # cmake, SDL3, SDL3_ttf and the shader tools
make run             # build and start the demo
```

MIT licence, see `LICENSE`.
