#version 450
#extension GL_GOOGLE_include_directive : require
// Soap bubble: rainbow bands of a thin film. p0: radius, intensity, scale, phase.
// Positions in lens points, as the reference; the layer is transparent past its margin around the lens.
#include "lens_common.glsl"

void main()
{
    vec2 position = lens_position();
    vec2 center = lens.zw;
    float radius = p0.x;
    float intensity = p0.y;
    float scale = p0.z;
    float phase = p0.w;
    vec2 toCenter = position - center;
    float dist = length(toCenter);
    float nd = dist / max(radius, 1e-5);
    float angle = atan(toCenter.y, toCenter.x);
    float clampedScale = max(scale, 0.1);
    float visibleFade = smoothstep(0.05, 0.3, nd) * (1.0 - smoothstep(0.74, 0.92, nd));
    float film = sin(nd * clampedScale * 6.2831853 + sin(angle * 3.0 + phase) * 1.2 + phase);
    float bands = film * 0.5 + 0.5;
    vec3 bubbleColor = 0.5 + 0.5 * cos(6.2831853 * (bands + vec3(0.0, 0.33, 0.67)));
    float strength = clamp(intensity, 0.0, 1.0) * visibleFade;
    vec4 base = layer_sample(position);
    base.rgb = mix(base.rgb, bubbleColor, strength * 0.42);
    base.rgb += bubbleColor * (strength * 0.18);
    o_color = base;
}
