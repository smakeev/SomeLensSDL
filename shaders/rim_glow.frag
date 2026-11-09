#version 450
#extension GL_GOOGLE_include_directive : require
// Rim glow: a warm to cool glow at the visible edge. p0: radius, intensity, width, softness.
// Positions in lens points, as the reference; the layer is transparent past its margin around the lens.
#include "lens_common.glsl"

void main()
{
    vec2 position = lens_position();
    vec2 center = lens.zw;
    float radius = p0.x;
    float intensity = p0.y;
    vec2 toCenter = position - center;
    float dist = length(toCenter);
    float nd = dist / max(radius, 1e-5);
    float clampedWidth = clamp(p0.z, 0.01, 0.5);
    float clampedSoftness = max(p0.w, 0.01);
    float visibleEdge = 0.70710678;
    float inner = max(visibleEdge - clampedWidth - clampedSoftness, 0.0);
    float peak = visibleEdge - clampedWidth * 0.25;
    float outer = visibleEdge + clampedSoftness * 0.75;
    float glowIn = smoothstep(inner, peak, nd);
    float glowOut = 1.0 - smoothstep(peak, outer, nd);
    float glow = clamp(glowIn * glowOut, 0.0, 1.0) * clamp(intensity, 0.0, 1.0);
    vec4 base = layer_sample(position);
    vec3 warmEdge = vec3(1.0, 0.96, 0.82);
    vec3 coolEdge = vec3(0.58, 0.78, 1.0);
    float colorShift = clamp((nd - 0.52) * 4.0, 0.0, 1.0);
    vec3 glowColor = mix(warmEdge, coolEdge, colorShift);
    vec3 lifted = mix(base.rgb, glowColor, glow * 0.55);
    base.rgb = lifted + glowColor * (glow * 0.35);
    o_color = base;
}
