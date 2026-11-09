#version 450
#extension GL_GOOGLE_include_directive : require
// Chromatic aberration: red and blue pulled apart along the radius. p0: radius, amount, falloff, edge only.
// Positions in lens points, as the reference; the layer is transparent past its margin around the lens.
#include "lens_common.glsl"

void main()
{
    vec2 position = lens_position();
    vec2 center = lens.zw;
    float radius = p0.x;
    float amount = p0.y;
    float falloff = p0.z;
    float edgeOnly = p0.w;
    vec2 toCenter = position - center;
    float dist = length(toCenter);
    float nd = dist / max(radius, 1e-5);
    vec2 direction = (dist > 1e-5) ? (toCenter / dist) : vec2(0.0);
    float clampedFalloff = max(falloff, 0.05);
    float edgeRamp = pow(clamp(nd, 0.0, 1.0), clampedFalloff);
    float fullRamp = edgeOnly > 0.5 ? smoothstep(0.55, 1.0, nd) * edgeRamp : edgeRamp;
    float offset = amount * fullRamp;
    vec2 channelOffset = direction * offset;
    vec4 base = layer_sample(position);
    vec4 redSample = layer_sample(position + channelOffset);
    vec4 blueSample = layer_sample(position - channelOffset);
    vec4 color = base;
    color.r = redSample.r;
    color.b = blueSample.b;
    color.a = base.a;
    o_color = color;
}
