#version 450
#extension GL_GOOGLE_include_directive : require
// Caustics: warm streaks of focused light, fading to the rim. p0: radius, intensity, scale, falloff.
// Positions in lens points, as the reference; the layer is transparent past its margin around the lens.
#include "lens_common.glsl"

void main()
{
    vec2 position = lens_position();
    vec2 center = lens.zw;
    float radius = p0.x;
    float intensity = p0.y;
    float scale = p0.z;
    float falloff = p0.w;
    vec2 toCenter = position - center;
    float dist = length(toCenter);
    float nd = dist / max(radius, 1e-5);
    vec2 uv = toCenter / max(radius, 1e-5);
    float clampedScale = max(scale, 0.1);
    float clampedFalloff = max(falloff, 0.05);
    float radialFade = pow(clamp(1.0 - nd, 0.0, 1.0), clampedFalloff);
    float rimFade = 1.0 - smoothstep(0.88, 1.0, nd);
    float waveA = sin((uv.x * 1.7 + uv.y * 0.7) * clampedScale + nd * 5.3);
    float waveB = sin((uv.x * -0.8 + uv.y * 1.9) * clampedScale - nd * 4.1);
    float streaks = pow(clamp((waveA + waveB) * 0.25 + 0.5, 0.0, 1.0), 5.0);
    float highlight = clamp(intensity, 0.0, 1.0) * streaks * radialFade * rimFade;
    vec4 base = layer_sample(position);
    vec3 tint = vec3(1.0, 0.94, 0.78);
    base.rgb += tint * highlight;
    o_color = base;
}
