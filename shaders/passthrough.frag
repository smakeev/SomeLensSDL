#version 450
// The texture as it is, tinted by the vertex colour: the GPU renderer's own textured fragment,
// used to check that a custom render state draws.
layout(location = 0) in vec4 v_color;
layout(location = 1) in vec2 v_uv;
layout(location = 0) out vec4 o_color;
layout(set = 2, binding = 0) uniform sampler2D u_texture;

void main()
{
    o_color = texture(u_texture, v_uv) * v_color;
}
