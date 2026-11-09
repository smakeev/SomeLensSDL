#version 450
#extension GL_GOOGLE_include_directive : require
// Colour glass: tints the content towards a glass colour. p0: red, green, blue, alpha.
// Positions in lens points, as the reference; the layer is transparent past its margin around the lens.
#include "lens_common.glsl"

void main()
{
    vec2 position = lens_position();
    vec4 base = layer_sample(position);
    vec3 glassColor = clamp(p0.rgb, 0.0, 1.0);
    float amount = clamp(p0.a, 0.0, 1.0);
    float luminance = dot(base.rgb, vec3(0.2126, 0.7152, 0.0722));
    vec3 tinted = mix(base.rgb, glassColor * (0.55 + luminance * 0.65), amount);
    base.rgb = tinted + glassColor * amount * 0.08;
    o_color = base;
}
