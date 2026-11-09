#version 450
#extension GL_GOOGLE_include_directive : require
// Magnification: enlarges the middle, easing back to 1 at the rim. p0: radius, scale, falloff, centre radius.
// Positions in lens points, as the reference; the layer is transparent past its margin around the lens.
#include "lens_common.glsl"

void main()
{
    vec2 position = lens_position();
    vec2 center = lens.zw;
    float radius = p0.x;
    vec2 toCenter = position - center;
    float dist = length(toCenter);
    float nd = dist / max(radius, 1e-5);
    float clampedScale = max(p0.y, 0.05);
    float clampedFalloff = max(p0.z, 0.05);
    float clampedCenterRadius = clamp(p0.w, 0.0, 0.95);
    float edgeProgress = smoothstep(clampedCenterRadius, 1.0, nd);
    float magnifyBlend = pow(1.0 - edgeProgress, clampedFalloff);
    float localScale = mix(1.0, 1.0 / clampedScale, magnifyBlend);
    o_color = layer_sample(center + toCenter * localScale);
}
