#version 450
#extension GL_GOOGLE_include_directive : require
// Ripple: concentric waves pushing the content along the radius. p0: radius, amplitude, frequency, phase; p1: falloff.
// Positions in lens points, as the reference; the layer is transparent past its margin around the lens.
#include "lens_common.glsl"

void main()
{
    vec2 position = lens_position();
    vec2 center = lens.zw;
    float radius = p0.x;
    float amplitude = p0.y;
    float frequency = p0.z;
    float phase = p0.w;
    float falloff = p1.x;
    vec2 toCenter = position - center;
    float dist = length(toCenter);
    float nd = dist / max(radius, 1e-5);
    vec2 direction = (dist > 1e-5) ? (toCenter / dist) : vec2(0.0);
    float clampedFalloff = max(falloff, 0.05);
    float edgeFade = 1.0 - smoothstep(0.82, 1.0, nd);
    float centerFade = smoothstep(0.0, 0.18, nd);
    float strength = pow(clamp(1.0 - nd, 0.0, 1.0), clampedFalloff) * edgeFade * centerFade;
    float wave = sin(nd * frequency * 6.2831853 + phase);
    float offset = amplitude * wave * strength;
    o_color = layer_sample(position + direction * offset);
}
