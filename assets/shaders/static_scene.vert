#version 460 core
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec2 a_texcoord;
layout (location = 2) in vec3 a_normal;

layout (location = 0) out vec3 f_pos;
layout (location = 1) out vec2 f_texcoord;
layout (location = 2) out vec3 f_normal;
layout (location = 3) out flat uint f_instanceID;

layout (set = 0, binding = 0) uniform TransformMatrices
{
    mat4 MVP;
} tm;

void main()
{
    gl_Position = tm.MVP * vec4(a_pos, 1.0f);
    f_pos = a_pos;
    f_texcoord = a_texcoord;
    f_normal = a_normal;
    f_instanceID = gl_InstanceIndex;
}