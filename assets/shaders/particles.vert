#version 460 core

layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec4 a_color;
layout (location = 2) in float a_size;

layout (location = 0) out vec4 g_color;
layout (location = 1) out float g_size;

void main()
{
    gl_Position = vec4(a_pos, 1.0f);
    g_color = a_color;
    g_size = a_size;
}