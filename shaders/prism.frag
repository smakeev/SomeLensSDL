#version 450
#extension GL_GOOGLE_include_directive : require
// Prism: colour split along facet directions. p0: radius, amount, facets, falloff.
// Positions in lens points, as the reference; the layer is transparent past its margin around the lens.
#include "lens_common.glsl"

void main()
{
    vec2 position = lens_position();
    vec2 center = lens.zw;
    float radius = p0.x;
    float amount = p0.y;
    float facets = p0.z;
    float falloff = p0.w;
    vec2 toCenter = position - center;
    float dist = length(toCenter);
    float nd = dist / max(radius, 1e-5);
    float angle = atan(toCenter.y, toCenter.x);
    float facetCount = max(facets, 3.0);
    float facetAngle = floor((angle + 3.14159265) / (6.2831853 / facetCount)) * (6.2831853 / facetCount);
    vec2 facetDirection = vec2(cos(facetAngle), sin(facetAngle));
    float edgeFade = smoothstep(0.18, 0.72, nd) * (1.0 - smoothstep(0.82, 1.0, nd));
    float strength = amount * pow(clamp(nd, 0.0, 1.0), max(falloff, 0.05)) * edgeFade;
    vec2 offset = facetDirection * strength;
    vec4 base = layer_sample(position);
    vec4 redSample = layer_sample(position + offset);
    vec4 greenSample = layer_sample(position - offset * 0.35);
    vec4 blueSample = layer_sample(position - offset);
    vec4 color = base;
    color.r = redSample.r;
    color.g = greenSample.g;
    color.b = blueSample.b;
    color.a = base.a;
    o_color = color;
}
