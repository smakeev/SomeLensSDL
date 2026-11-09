#pragma once

#include <SDL3/SDL.h>

#include <cstddef>
#include <cstdint>

namespace somelens {

/** @brief One compiled fragment shader in the formats the GPU renderer may want. */
struct ShaderBinary {
    const unsigned char* spirv = nullptr;
    std::size_t spirv_size = 0;
    const unsigned char* msl = nullptr;  // NUL-terminated
    std::size_t msl_size = 0;
    std::uint32_t uniform_buffers = 0;
};

/**
 * @brief A fragment shader as a render state of SDL's GPU renderer: set it, and the renderer's next draws run the
 * shader instead of its own (the texture drawn is the shader's sampler 0, the uniforms its buffer 0).
 */
class ShaderProgram {
public:
    ShaderProgram() = default;
    ~ShaderProgram();

    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;
    ShaderProgram(ShaderProgram&& other) noexcept;
    ShaderProgram& operator=(ShaderProgram&& other) noexcept;

    /** @brief Builds the shader for the renderer's GPU device; false (SDL_GetError says why) when it cannot. */
    bool create(SDL_Renderer* renderer, const ShaderBinary& binary);
    void destroy();

    /** @brief Uniform data for the next draws: copied, so a local struct is fine. */
    void set_uniforms(const void* data, std::uint32_t size);

    [[nodiscard]] SDL_GPURenderState* state() const noexcept { return state_; }

private:
    SDL_GPUDevice* device_ = nullptr;
    SDL_GPUShader* shader_ = nullptr;
    SDL_GPURenderState* state_ = nullptr;
};

/** @brief A window renderer that runs on SDL's GPU API (Vulkan, Metal or Direct3D 12), so custom shaders work. */
[[nodiscard]] SDL_Renderer* create_gpu_renderer(SDL_Window* window);

} // namespace somelens
