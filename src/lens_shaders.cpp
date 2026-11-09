#include "lens_shaders.hpp"

#include "generated/caustics.msl.h"
#include "generated/caustics.spv.h"
#include "generated/chromatic_aberration.msl.h"
#include "generated/chromatic_aberration.spv.h"
#include "generated/color_glass.msl.h"
#include "generated/color_glass.spv.h"
#include "generated/frosted_blur.msl.h"
#include "generated/frosted_blur.spv.h"
#include "generated/magnification.msl.h"
#include "generated/magnification.spv.h"
#include "generated/noise.msl.h"
#include "generated/noise.spv.h"
#include "generated/prism.msl.h"
#include "generated/prism.spv.h"
#include "generated/refraction.msl.h"
#include "generated/refraction.spv.h"
#include "generated/rim_glow.msl.h"
#include "generated/rim_glow.spv.h"
#include "generated/ripple.msl.h"
#include "generated/ripple.spv.h"
#include "generated/soap_bubble.msl.h"
#include "generated/soap_bubble.spv.h"
#include "generated/sparkle.msl.h"
#include "generated/sparkle.spv.h"
#include "generated/twirl.msl.h"
#include "generated/twirl.spv.h"
#include "generated/vignette.msl.h"
#include "generated/vignette.spv.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <type_traits>

namespace somelens {
namespace {

template <std::size_t N, std::size_t M>
constexpr ShaderBinary binary(const unsigned char (&spirv)[N], const unsigned char (&msl)[M])
{
    return ShaderBinary{spirv, N, msl, M, 1};
}

/* The values of each kind that blend in a morph and make its signature, in order. */
constexpr auto members(const RefractionShader*)
{
    return std::array{&RefractionShader::refraction, &RefractionShader::edge_reflection};
}
constexpr auto members(const ChromaticAberrationShader*)
{
    return std::array{&ChromaticAberrationShader::amount, &ChromaticAberrationShader::falloff};
}
constexpr auto members(const FrostedBlurShader*)
{
    return std::array{&FrostedBlurShader::radius, &FrostedBlurShader::intensity, &FrostedBlurShader::edge_bias};
}
constexpr auto members(const MagnificationShader*)
{
    return std::array{&MagnificationShader::scale, &MagnificationShader::falloff, &MagnificationShader::center_radius};
}
constexpr auto members(const RippleShader*)
{
    return std::array{&RippleShader::amplitude, &RippleShader::frequency, &RippleShader::phase, &RippleShader::falloff};
}
constexpr auto members(const TwirlShader*)
{
    return std::array{&TwirlShader::angle, &TwirlShader::falloff, &TwirlShader::center_radius};
}
constexpr auto members(const CausticsShader*)
{
    return std::array{&CausticsShader::intensity, &CausticsShader::scale, &CausticsShader::falloff};
}
constexpr auto members(const VignetteShader*)
{
    return std::array{&VignetteShader::intensity, &VignetteShader::radius, &VignetteShader::softness};
}
constexpr auto members(const RimGlowShader*)
{
    return std::array{&RimGlowShader::intensity, &RimGlowShader::width, &RimGlowShader::softness};
}
constexpr auto members(const SparkleShader*)
{
    return std::array{&SparkleShader::intensity, &SparkleShader::scale, &SparkleShader::threshold};
}
constexpr auto members(const PrismShader*)
{
    return std::array{&PrismShader::amount, &PrismShader::facets, &PrismShader::falloff};
}
constexpr auto members(const ColorGlassShader*)
{
    return std::array{&ColorGlassShader::red, &ColorGlassShader::green, &ColorGlassShader::blue,
                      &ColorGlassShader::alpha};
}
constexpr auto members(const NoiseShader*)
{
    return std::array{&NoiseShader::red, &NoiseShader::green, &NoiseShader::blue, &NoiseShader::alpha,
                      &NoiseShader::density};
}
constexpr auto members(const SoapBubbleShader*)
{
    return std::array{&SoapBubbleShader::intensity, &SoapBubbleShader::scale, &SoapBubbleShader::phase};
}

} // namespace

ShaderBinary shader_binary(const LensShader& shader)
{
    static constexpr std::array<ShaderBinary, std::variant_size_v<LensShader>> kBinaries{
        binary(refraction_spv, refraction_msl),
        binary(chromatic_aberration_spv, chromatic_aberration_msl),
        binary(frosted_blur_spv, frosted_blur_msl),
        binary(magnification_spv, magnification_msl),
        binary(ripple_spv, ripple_msl),
        binary(twirl_spv, twirl_msl),
        binary(caustics_spv, caustics_msl),
        binary(vignette_spv, vignette_msl),
        binary(rim_glow_spv, rim_glow_msl),
        binary(sparkle_spv, sparkle_msl),
        binary(prism_spv, prism_msl),
        binary(color_glass_spv, color_glass_msl),
        binary(noise_spv, noise_msl),
        binary(soap_bubble_spv, soap_bubble_msl),
    };
    return kBinaries[shader.index()];
}

float shader_sample_offset(const LensShader& shader, float radius)
{
    return std::visit(
        [&](const auto& s) -> float {
            using Shader = std::decay_t<decltype(s)>;
            if constexpr (std::is_same_v<Shader, RefractionShader>) {
                return radius * 1.15f * s.refraction + 10.0f;
            } else if constexpr (std::is_same_v<Shader, ChromaticAberrationShader>) {
                return std::max(s.amount, 0.0f) + 2.0f;
            } else if constexpr (std::is_same_v<Shader, FrostedBlurShader>) {
                return std::max(s.radius, 0.0f) + 2.0f;
            } else if constexpr (std::is_same_v<Shader, RippleShader>) {
                return std::fabs(s.amplitude) + 2.0f;
            } else if constexpr (std::is_same_v<Shader, TwirlShader>) {
                return radius * std::min(std::fabs(s.angle), 3.14159265f) * 0.35f + 2.0f;
            } else if constexpr (std::is_same_v<Shader, PrismShader>) {
                return std::max(s.amount, 0.0f) + 2.0f;
            } else {
                return 2.0f;
            }
        },
        shader);
}

ShaderUniforms shader_uniforms(const LensShader& shader, float width, float height, float radius, float margin,
                               SDL_FPoint layer, SDL_FPoint atlas)
{
    ShaderUniforms u{{width, height, width / 2.0f, height / 2.0f}, {margin, margin, layer.x, layer.y},
                     {atlas.x, atlas.y, 0.0f, 0.0f}, {radius, 0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f}};
    std::visit(
        [&](const auto& s) {
            using Shader = std::decay_t<decltype(s)>;
            if constexpr (std::is_same_v<Shader, ChromaticAberrationShader>) {
                u.p0[1] = s.amount;
                u.p0[2] = s.falloff;
                u.p0[3] = s.edge_only ? 1.0f : 0.0f;
            } else if constexpr (std::is_same_v<Shader, ColorGlassShader>) {
                u.p0[0] = s.red;
                u.p0[1] = s.green;
                u.p0[2] = s.blue;
                u.p0[3] = s.alpha;
            } else if constexpr (std::is_same_v<Shader, NoiseShader>) {
                u.p0[1] = s.red;
                u.p0[2] = s.green;
                u.p0[3] = s.blue;
                u.p1[0] = s.alpha;
                u.p1[1] = s.density;
            } else if constexpr (std::is_same_v<Shader, RippleShader>) {
                u.p0[1] = s.amplitude;
                u.p0[2] = s.frequency;
                u.p0[3] = s.phase;
                u.p1[0] = s.falloff;
            } else {
                // The rest take the radius and then their values in order, as the reference does.
                std::size_t i = 1;
                for (const auto member : members(&s)) {
                    u.p0[i++] = s.*member;
                }
            }
        },
        shader);
    return u;
}

std::vector<float> shader_signature(const LensShader& shader)
{
    std::vector<float> values{static_cast<float>(shader.index())};
    std::visit(
        [&](const auto& s) {
            for (const auto member : members(&s)) {
                values.push_back(s.*member);
            }
            if constexpr (std::is_same_v<std::decay_t<decltype(s)>, ChromaticAberrationShader>) {
                values.push_back(s.edge_only ? 1.0f : 0.0f);
            }
        },
        shader);
    return values;
}

LensShader blended(const LensShader& from, const LensShader& to, float t)
{
    if (from.index() != to.index()) {
        return to;
    }
    return std::visit(
        [&](const auto& end) -> LensShader {
            using Shader = std::decay_t<decltype(end)>;
            const Shader& start = std::get<Shader>(from);
            Shader result = end;  // what does not blend (edge only) is the end's, as the reference does
            for (const auto member : members(&end)) {
                result.*member = start.*member + (end.*member - start.*member) * t;
            }
            return result;
        },
        to);
}

} // namespace somelens
