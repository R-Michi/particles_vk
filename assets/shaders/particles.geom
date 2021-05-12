#version 460 core

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

layout (location = 0) in vec4 g_color[];
layout (location = 1) in float g_size[];

layout (location = 0) out vec4 f_color;
layout (location = 1) out vec2 f_texcoord;

layout (set = 0, binding = 0) uniform TransformMatrices
{
    mat4 view;
    mat4 projection;
} tm;

void main()
{
    mat4 VP = tm.projection * tm.view;
    vec3 cam_right = vec3(tm.view[0][0], tm.view[1][0], tm.view[2][0]);  // get right vector
    vec3 cam_up = vec3(tm.view[0][1], tm.view[1][1], tm.view[2][1]);     // get up vector
    vec3 pos = gl_in[0].gl_Position.xyz;

    // first vertex
    gl_Position = VP * vec4(pos + cam_right * g_size[0]*+0.5f + cam_up * g_size[0]*-0.5f, 1.0f);
    f_color = g_color[0];
    f_texcoord = vec2(1.0f, 0.0f);
    EmitVertex();

    // second vertex
    gl_Position = VP * vec4(pos + cam_right * g_size[0]*+0.5f + cam_up * g_size[0]*+0.5f, 1.0f);
    f_color = g_color[0];
    f_texcoord = vec2(1.0f, 1.0f);
    EmitVertex();

    // third vertex
    gl_Position = VP * vec4(pos + cam_right * g_size[0]*-0.5f + cam_up * g_size[0]*-0.5f, 1.0f);
    f_color = g_color[0];
    f_texcoord = vec2(0.0f, 0.0f);
    EmitVertex();

    // forth vertex
    gl_Position = VP * vec4(pos + cam_right * g_size[0]*-0.5f + cam_up * g_size[0]*+0.5f, 1.0f);
    f_color = g_color[0];
    f_texcoord = vec2(0.0f, 1.0f);
    EmitVertex();
}