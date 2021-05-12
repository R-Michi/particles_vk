#version 460 core

layout (location = 0) in vec4 f_color;
layout (location = 1) in vec2 f_texcoord;

layout (location = 0) out vec4 out_color;


layout (set = 0, binding = 1) uniform sampler2D tex;

void main()
{
    vec4 res_color = texture(tex, f_texcoord) * f_color;
    if(res_color.a < 0.1f)
        discard;
    out_color = res_color;
}