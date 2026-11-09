#include "somelens/shader_program.hpp"

#include <utility>

namespace somelens {

ShaderProgram::~ShaderProgram()
{
    destroy();
}

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept
    : device_(std::exchange(other.device_, nullptr)), shader_(std::exchange(other.shader_, nullptr))
    , state_(std::exchange(other.state_, nullptr))
{
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept
{
    if (this != &other) {
        destroy();
        device_ = std::exchange(other.device_, nullptr);
        shader_ = std::exchange(other.shader_, nullptr);
        state_ = std::exchange(other.state_, nullptr);
    }
    return *this;
}

bool ShaderProgram::create(SDL_Renderer* renderer, const ShaderBinary& binary)
{
    destroy();
    device_ = SDL_GetGPURendererDevice(renderer);
    if (device_ == nullptr) {
        return false;
    }
    SDL_GPUShaderCreateInfo info{};
    const SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(device_);
    if ((formats & SDL_GPU_SHADERFORMAT_SPIRV) != 0 && binary.spirv != nullptr) {
        info.format = SDL_GPU_SHADERFORMAT_SPIRV;
        info.code = binary.spirv;
        info.code_size = binary.spirv_size;
        info.entrypoint = "main";
    } else if ((formats & SDL_GPU_SHADERFORMAT_MSL) != 0 && binary.msl != nullptr) {
        // spirv-cross names the entry point main0.
        info.format = SDL_GPU_SHADERFORMAT_MSL;
        info.code = binary.msl;
        info.code_size = binary.msl_size;
        info.entrypoint = "main0";
    } else {
        SDL_SetError("somelens: the GPU device takes neither SPIR-V nor MSL");
        return false;
    }
    info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    info.num_samplers = 1;
    info.num_uniform_buffers = binary.uniform_buffers;
    shader_ = SDL_CreateGPUShader(device_, &info);
    if (shader_ == nullptr) {
        return false;
    }
    SDL_GPURenderStateCreateInfo create{};
    create.fragment_shader = shader_;
    state_ = SDL_CreateGPURenderState(renderer, &create);
    return state_ != nullptr;
}

void ShaderProgram::destroy()
{
    if (state_ != nullptr) {
        SDL_DestroyGPURenderState(state_);
        state_ = nullptr;
    }
    if (shader_ != nullptr && device_ != nullptr) {
        SDL_ReleaseGPUShader(device_, shader_);
    }
    shader_ = nullptr;
    device_ = nullptr;
}

void ShaderProgram::set_uniforms(const void* data, std::uint32_t size)
{
    if (state_ != nullptr) {
        SDL_SetGPURenderStateFragmentUniforms(state_, 0, data, size);
    }
}

SDL_Renderer* create_gpu_renderer(SDL_Window* window)
{
    return SDL_CreateGPURenderer(nullptr, window);
}

} // namespace somelens
