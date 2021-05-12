#version 460 core
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 f_pos;
layout (location = 1) in vec2 f_texcoord;
layout (location = 2) in vec3 f_normal;
layout (location = 3) in flat uint f_instanceID;

layout (location = 0) out vec4 out_color;

layout(set = 0, binding = 1) uniform sampler2D tex[2];

void main()
{
    out_color = texture(tex[f_instanceID], f_texcoord);
}