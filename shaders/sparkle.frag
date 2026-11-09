#version 450
#extension GL_GOOGLE_include_directive : require
// Sparkle: small star glints scattered near the rim. p0: radius, intensity, scale, threshold.
// Positions in lens points, as the reference; the layer is transparent past its margin around the lens.
#include "lens_common.glsl"

void main()
{
    vec2 position = lens_position();
    vec2 center = lens.zw;
    float radius = p0.x;
    float intensity = p0.y;
    float scale = p0.z;
    float threshold = p0.w;
    vec2 toCenter = position - center;
    float dist = length(toCenter);
    float nd = dist / max(radius, 1e-5);
    vec2 uv = toCenter / max(radius, 1e-5);
    float clampedScale = max(scale, 1.0);
    vec2 cell = floor((uv + 1.0) * clampedScale);
    float cellHash = fract(sin(dot(cell, vec2(12.9898, 78.233))) * 43758.5453);
    vec2 local = ((uv + 1.0) * clampedScale) - cell - 0.5;
    float starShape = 1.0 - smoothstep(0.0, 0.42, length(local));
    starShape *= max(abs(local.x), abs(local.y)) < 0.36 ? 1.0 : 0.0;
    float rimBias = smoothstep(0.18, 0.72, nd) * (1.0 - smoothstep(0.78, 1.0, nd));
    float sparkleMask = smoothstep(clamp(threshold, 0.0, 0.98), 1.0, cellHash);
    float glint = starShape * sparkleMask * rimBias * clamp(intensity, 0.0, 1.0);
    vec4 base = layer_sample(position);
    vec3 sparkleColor = vec3(1.0, 0.98, 0.88);
    base.rgb = mix(base.rgb, sparkleColor, glint * 0.65) + sparkleColor * (glint * 0.45);
    o_color = base;
}
