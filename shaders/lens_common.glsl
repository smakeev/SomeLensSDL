// Shared by every lens shader: the renderer's inputs, the previous step's output as the layer, and the uniforms
// (all in points): the lens size and centre, the layer's margin around the lens and its size, then the shader's own
// values in p0, p1. Positions are in lens points: (0, 0) is the lens's top left, the margin lies outside it.
layout(location = 0) in vec4 v_color;
layout(location = 1) in vec2 v_uv;
layout(location = 0) out vec4 o_color;
layout(set = 2, binding = 0) uniform sampler2D u_texture;
layout(set = 3, binding = 0) uniform Uniforms {
    vec4 lens;   // size (x, y), centre (z, w)
    vec4 frame;  // margin (x, y), layer size (z, w)
    vec4 p0;
    vec4 p1;
};

vec4 layer_sample(vec2 p)
{
    vec2 q = p + frame.xy;
    if (q.x < 0.0 || q.y < 0.0 || q.x >= frame.z || q.y >= frame.w) {
        return vec4(0.0);
    }
    return texture(u_texture, q / frame.zw);
}

vec2 lens_position()
{
    return v_uv * frame.zw - frame.xy;
}
