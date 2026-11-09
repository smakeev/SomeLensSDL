#pragma once

#include "somelens/lens_settings.hpp"
#include "somelens/shader_program.hpp"

#include <vector>

namespace somelens {

/**
 * @brief What every lens shader takes (shaders/lens_common.glsl), in points: lens size and centre, the margin of the
 * layer around the lens and the layer's size, the size of the texture the layer is the top left part of, then its
 * values.
 */
struct ShaderUniforms {
    float lens[4];
    float frame[4];
    float atlas[4];
    float p0[4];
    float p1[4];
};

/** @brief The compiled shader of this kind of step. */
[[nodiscard]] ShaderBinary shader_binary(const LensShader& shader);

/**
 * @brief The step's uniforms for a lens of `width` by `height` points and shader radius `radius` points, on a layer
 * of `layer` points with `margin` points around the lens, the top left part of a texture of `atlas` points.
 */
[[nodiscard]] ShaderUniforms shader_uniforms(const LensShader& shader, float width, float height, float radius,
                                             float margin, SDL_FPoint layer, SDL_FPoint atlas);

/**
 * @brief How far past the lens the step samples, in points, for a lens of shader radius `radius` and shorter side
 * `shorter`: what its formula can actually reach, so the margin taken around the crop stays small.
 */
[[nodiscard]] float shader_sample_offset(const LensShader& shader, float radius, float shorter);

/** @brief The step's kind and values, to tell whether a chain changed. */
[[nodiscard]] std::vector<float> shader_signature(const LensShader& shader);

/** @brief `to` with its values moved from `from`'s by `t`; `to` as it is when the two are not the same kind. */
[[nodiscard]] LensShader blended(const LensShader& from, const LensShader& to, float t);

} // namespace somelens
