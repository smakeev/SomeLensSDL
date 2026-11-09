#version 450
#extension GL_GOOGLE_include_directive : require
// Twirl: turns the content around the centre, most in the middle. p0: radius, angle, falloff, centre radius.
// Positions in lens points, as the reference; the layer is transparent past its margin around the lens.
#include "lens_common.glsl"

void main()
{
    vec2 position = lens_position();
    vec2 center = lens.zw;
    float radius = p0.x;
    float angle = p0.y;
    vec2 toCenter = position - center;
    float dist = length(toCenter);
    float nd = dist / max(radius, 1e-5);
    float clampedFalloff = max(p0.z, 0.05);
    float clampedCenterRadius = clamp(p0.w, 0.0, 0.95);
    float edgeProgress = smoothstep(clampedCenterRadius, 1.0, nd);
    float strength = pow(1.0 - edgeProgress, clampedFalloff);
    float localAngle = -angle * strength;
    float s = sin(localAngle);
    float c = cos(localAngle);
    vec2 rotated = vec2(toCenter.x * c - toCenter.y * s, toCenter.x * s + toCenter.y * c);
    o_color = layer_sample(center + rotated);
}
