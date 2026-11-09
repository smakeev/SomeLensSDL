#version 450
#extension GL_GOOGLE_include_directive : require
// Lens refraction: magnifies the middle, bends the content towards the rim and mirrors a narrow edge.
// Positions in lens points, as the reference; the layer is transparent past its margin around the lens.
#include "lens_common.glsl"

void main()
{
    vec2 position = lens_position();
    vec2 center = lens.zw;
    float radius = p0.x;
    float refract = p0.y;
    float edge = p0.z;

    vec2 toCenter = position - center;
    float dist = length(toCenter);

    // Extend effect area beyond visible radius for early visibility
    float effectRadius = radius * 1.15;
    if (dist >= effectRadius) {
        o_color = vec4(0.0);
        return;
    }

    // Normalized distance from center (0 = center, 1 = rim, >1 = extended area)
    float nd = dist / max(radius, 1e-5);

    // Fade out alpha beyond the main radius
    float alphaFade = 1.0;
    if (dist > radius) {
        alphaFade = 1.0 - smoothstep(radius, effectRadius, dist);
    }

    // Direction from lens center to the sampled point
    vec2 normal = (dist > 1e-5) ? (toCenter / dist) : vec2(0.0);

    // Refraction: stronger magnification near the center with a narrow transition zone
    float magnifyStrength = refract * 0.5;
    float centerRegion = 0.55;
    float magnifyFalloff = smoothstep(0.0, centerRegion, nd);
    float scale = 1.0 - magnifyStrength * (1.0 - magnifyFalloff);

    float refractionBlend = smoothstep(centerRegion, 0.95, nd);
    float refractScale = refract * radius * refractionBlend * 0.35;
    vec2 refractPos = center + toCenter * scale - normal * refractScale;
    vec4 refracted = layer_sample(refractPos);
    if (refracted.a < 0.001) {
        refracted = layer_sample(center + toCenter * scale);
    }

    // Reflection: a narrow reflective rim
    float reflectScale = refract * radius * 0.02;
    vec4 reflected = layer_sample(position + normal * reflectScale);
    if (reflected.a < 0.001) {
        reflected = refracted;
    }

    float clampedEdge = clamp(edge, 0.0, 1.0);
    float rimWidth = 0.003 + (1.0 - clampedEdge) * 0.01;
    float rimInner = 1.0 - rimWidth;
    float rimOuter = 1.0 - rimWidth * 0.25;
    float rimMix = smoothstep(rimInner, rimOuter, nd) * clampedEdge;
    vec4 mixed = mix(refracted, reflected, rimMix);

    mixed.rgb += smoothstep(0.99, 1.0, nd) * 0.06;
    mixed.a = alphaFade;
    o_color = mixed;
}
