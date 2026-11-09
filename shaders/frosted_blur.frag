#version 450
#extension GL_GOOGLE_include_directive : require
// Frosted blur: nine taps, wider towards the rim. p0: radius, blur radius, intensity, edge bias.
// Positions in lens points, as the reference; the layer is transparent past its margin around the lens.
#include "lens_common.glsl"

void main()
{
    vec2 position = lens_position();
    vec2 center = lens.zw;
    float radius = p0.x;
    float blurRadius = p0.y;
    float intensity = p0.z;
    float edgeBias = p0.w;
    vec2 toCenter = position - center;
    float dist = length(toCenter);
    float nd = dist / max(radius, 1e-5);
    float clampedIntensity = clamp(intensity, 0.0, 1.0);
    float clampedEdgeBias = clamp(edgeBias, 0.0, 1.0);
    float blurRamp = mix(1.0, smoothstep(0.15, 1.0, nd), clampedEdgeBias);
    float spread = max(blurRadius, 0.0) * blurRamp;
    vec4 base = layer_sample(position);
    vec4 sum = base * 0.20;
    sum += layer_sample(position + vec2(spread, 0.0)) * 0.10;
    sum += layer_sample(position + vec2(-spread, 0.0)) * 0.10;
    sum += layer_sample(position + vec2(0.0, spread)) * 0.10;
    sum += layer_sample(position + vec2(0.0, -spread)) * 0.10;
    float diagonalSpread = spread * 0.70710678;
    sum += layer_sample(position + vec2(diagonalSpread, diagonalSpread)) * 0.10;
    sum += layer_sample(position + vec2(-diagonalSpread, diagonalSpread)) * 0.10;
    sum += layer_sample(position + vec2(diagonalSpread, -diagonalSpread)) * 0.10;
    sum += layer_sample(position + vec2(-diagonalSpread, -diagonalSpread)) * 0.10;
    vec4 color = mix(base, sum, clampedIntensity);
    color.a = base.a;
    o_color = color;
}
