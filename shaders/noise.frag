#version 450
#extension GL_GOOGLE_include_directive : require
// Noise: a grain of the given colour, one cell a point. p0: radius, red, green, blue; p1: alpha, density.
// Positions in lens points, as the reference; the layer is transparent past its margin around the lens.
#include "lens_common.glsl"

void main()
{
    vec2 position = lens_position();
    vec2 center = lens.zw;
    float radius = p0.x;
    float alpha = p1.x;
    float density = p1.y;
    vec2 toCenter = position - center;
    float dist = length(toCenter);
    float nd = dist / max(radius, 1e-5);
    float visibleFade = 1.0 - smoothstep(0.72, 0.9, nd);
    float grain = fract(sin(dot(floor(position), vec2(12.9898, 78.233))) * 43758.5453);
    float mask = smoothstep(1.0 - clamp(density, 0.0, 1.0), 1.0, grain) * visibleFade;
    float signedNoise = (grain * 2.0 - 1.0) * mask * clamp(alpha, 0.0, 1.0);
    vec4 base = layer_sample(position);
    vec3 noiseColor = clamp(p0.yzw, 0.0, 1.0);
    base.rgb = mix(base.rgb, noiseColor, abs(signedNoise) * 0.65);
    base.rgb += noiseColor * (signedNoise * 0.28);
    o_color = base;
}
