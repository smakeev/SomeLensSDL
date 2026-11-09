#version 450
#extension GL_GOOGLE_include_directive : require
// Vignette: darkens towards the rim with a little contrast. p0: radius, intensity, vignette radius, softness.
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
    float inner = clamp(p0.z, 0.0, 0.98);
    float outer = clamp(inner + max(p0.w, 0.01), inner + 0.01, 1.05);
    float edge = smoothstep(inner, outer, nd);
    float darken = clamp(intensity, 0.0, 1.0) * edge * edge;
    float contrast = 1.0 + darken * 0.18;
    vec4 base = layer_sample(position);
    vec3 shadow = vec3(0.22, 0.28, 0.36);
    vec3 contrasted = (base.rgb - 0.5) * contrast + 0.5;
    base.rgb = mix(contrasted, shadow, darken * 0.72);
    o_color = base;
}
