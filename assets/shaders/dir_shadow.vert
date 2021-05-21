#version 460 core

layout (location = 0) in vec3 a_pos;

layout (set = 0, binding = 0) uniform TransformMatrices
{
    mat4 MVP;
} tm;

void main()
{
    gl_Position = tm.MVP * vec4(a_pos, 1.0f);
}