#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#define M_PI    3.1415926535897f
#define M_2PI   6.2831853071794f
#define DISPLAY_SHADOW_MAP 0

// ---------------------------- STRUCTS ----------------------------
struct DirectionalLight
{
    vec3 direction;
    vec3 intensity;
};

struct Material
{
    vec3 albedo;
    float roughness;
    float metallic;
    float alpha;
};

// ---------------------------- INPUT ----------------------------

layout (location = 0) in vec3 f_pos;
layout (location = 1) in vec2 f_texcoord;
layout (location = 2) in vec3 f_normal;
layout (location = 3) in flat uint f_instanceID;
layout (location = 4) in vec4 f_light_space_pos;

// ---------------------------- OUTPUT ----------------------------

layout (location = 0) out vec4 out_color;

// ---------------------------- UNIFORMS ----------------------------

// samplers
layout (set = 0, binding = 1) uniform sampler2D tex[2];

// directional shadow maps
layout (set = 0, binding = 5) uniform sampler2DShadow directional_shadow_map;

// directional lights
layout (set = 0, binding = 2) uniform DirectionalLightUniform
{
    DirectionalLight lights[1];
} dlu;

// materials
layout (set = 0, binding = 3) uniform MaterialUniform
{
    Material materials[2];
} mtlu;

// other variables
layout (set = 0, binding = 4) uniform FragmentVariablesUniform
{
    vec3 cam_pos;
    float shadow_penumbra_size;
    int shadow_samples;
} fragment_variables;

// ---------------------------- FUNCTIONS ----------------------------

float distribution_GGX(vec3 N, vec3 H, float r)
{
    const float a = r*r;
    const float a2 = a*a;
    const float NdotH = max(dot(N, H), 0.0f);
    const float NdotH2 = NdotH * NdotH;
    const float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);

    return a2 / (M_PI * denom * denom);
}

float geometry_schlick_GGX(float NdotV, float r)
{
    const float roughness = (r + 1.0f);
    const float k = (roughness*roughness) / 8.0f;
    const float denom = NdotV * (1.0f - k) + k;
    return NdotV / denom;
}

float geometry_smith(vec3 N, vec3 V, vec3 L, float r)
{
    const float NdotV = max(dot(N, V), 0.0f);
    const float NdotL = max(dot(N, L), 0.0f);
    const float ggx1 = geometry_schlick_GGX(NdotV, r);
    const float ggx2 = geometry_schlick_GGX(NdotL, r);
    return ggx1 * ggx2;
}

vec3 frensel_schlick(vec3 H, vec3 V, vec3 F0)
{
    const float HdotV = 1.0f - max(dot(H, V), 0.0f);
    return F0 + (1.0f - F0) * pow(max(HdotV, 0.0f), 5.0f);
}

vec3 directional_light(DirectionalLight L, Material M, vec3 V, vec3 N)
{
    vec3 _F0 = vec3(0.04f);
    vec3 F0 = mix(_F0, M.albedo, M.metallic);

    L.direction = normalize(-L.direction);
    V = normalize(V);

    vec3 H = normalize(L.direction + V);
    vec3 radiance = L.intensity;

    const float NDF = distribution_GGX(N, H, M.roughness);
    const float G = geometry_smith(N, V, L.direction, M.roughness);
    vec3 F = frensel_schlick(H, V, F0);

    vec3 kd = vec3(1.0f) - F;
    kd *= 1.0f - M.metallic;

    vec3 numerator = NDF * F * G;
    const float denom = 4.0f * max(dot(V, N), 0.0f) * max(dot(L.direction, N), 0.0f);
    vec3 specular = numerator / max(denom, 0.001f);

    float NdotL = max(dot(N, L.direction), 0.0f);
    return (kd * (M.albedo / M_PI) + specular) * radiance * NdotL;
}

vec2 vogeldisk_sample(int sample_index, int samples_count, float phi, float disk_radius, vec2 center)
{
    float theta = 2.4f * float(sample_index) + phi;
    float r = sqrt(float(sample_index) + 0.5f) / sqrt(float(samples_count));
    vec2 u = r * vec2(cos(theta), sin(theta));
    return center + u * disk_radius;
}

float interleaved_gradient_noise(vec2 pos)
{
    vec3 seed = vec3(0.06711056f, 0.00583715f, 52.9829189f);
    return fract(seed.z * fract(dot(pos, seed.xy)));
}

int get_test_sample_count(int count)
{
    if(count > 16)
        return 8;
    else if(count > 8)
        return 6;
    else if(count > 4)
        return 4;
    return 0;
}

float shadow_value(vec4 light_space_pos, int samples, float penumbra_size, float NdotL)
{
    const int test_samples = get_test_sample_count(samples);
    const int test_samples_end = samples - test_samples;

    // make a perspective divide -> transforms light space coordinates to NDC coodinates range [-1, +1]
    vec3 light_project_coords = light_space_pos.xyz / light_space_pos.w;
    if(light_project_coords.z > 1.0f)
        return 1.0f;
    
    // texture coordinates have a range of [0, +1]
    light_project_coords.xy = light_project_coords.xy * 0.5f + 0.5f;

    float visibility = 0.0f;
    vec3 sm_coords = light_project_coords;
    const float noise = interleaved_gradient_noise(gl_FragCoord.xy) * M_2PI;

    // take a few test samples
    for(int i = samples - 1; i >= test_samples_end; i--)
    {
        sm_coords.xy = vogeldisk_sample(i, samples, noise, penumbra_size, light_project_coords.xy);
        visibility += texture(directional_shadow_map, sm_coords);
    }

    if(visibility == test_samples)          return 1.0f;    // completely in light
    if(visibility == 0.0f || NdotL < 0.0f)  return 0.0f;    // compelely in shadow

    // at this point in the code, we are at the penumbra of the shadow 
    // only do the expensive sampling at the penumbra region
    // expensive sampling reduced to as few pixels as possible
    for(int i = test_samples_end - 1; i >= 0; i--)
    {
        sm_coords.xy = vogeldisk_sample(i, samples, noise, penumbra_size, light_project_coords.xy);
        visibility += texture(directional_shadow_map, sm_coords);
    }

    return visibility / samples; // visibility range [0, +1]
}

// ---------------------------- MAIN ----------------------------

void main()
{
#if DISPLAY_SHADOW_MAP
    out_color = vec4(texture(tex[f_instanceID], f_texcoord).rrr, 1.0f);
#else
    vec4 tex_color = texture(tex[f_instanceID], f_texcoord);
    vec3 view_vec = fragment_variables.cam_pos - f_pos;
    float NdotL = dot(f_normal, normalize(-dlu.lights[0].direction));

    float shadow = shadow_value(f_light_space_pos, fragment_variables.shadow_samples, fragment_variables.shadow_penumbra_size, NdotL);

    vec3 light_intensity = directional_light(dlu.lights[0], mtlu.materials[f_instanceID], view_vec, f_normal) * shadow;
    vec3 hdr_color = (light_intensity + mtlu.materials[f_instanceID].albedo) * tex_color.rgb;
    vec3 ldr_color = hdr_color / (hdr_color + vec3(1.0f));

    out_color = vec4(ldr_color, tex_color.a * mtlu.materials[f_instanceID].alpha);
#endif
}