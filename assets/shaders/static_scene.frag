#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#define M_PI 3.14159265358979323846f
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

// jitter map
layout (set = 0, binding = 6) uniform sampler3D jitter_map;

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
    int kernel_radius;
    float penumbra_size;
    vec2 jitter_scale;
    float inv_shadow_map_size;
} fragment_variables;

// ---------------------------- FUNCTIONS ----------------------------

float distribution_GGX(vec3 N, vec3 H, float r)
{
    const float a       = r*r;
    const float a2      = a*a;
    const float NdotH   = max(dot(N, H), 0.0f);
    const float NdotH2  = NdotH * NdotH;
    const float denom   = (NdotH2 * (a2 - 1.0f) + 1.0f);

    return a2 / (M_PI * denom * denom);
}

float geometry_schlick_GGX(float NdotV, float r)
{
    const float roughness   = (r + 1.0f);
    const float k           = (roughness*roughness) / 8.0f;
    const float denom       = NdotV * (1.0f - k) + k;
    return NdotV / denom;
}

float geometry_smith(vec3 N, vec3 V, vec3 L, float r)
{
    const float NdotV   = max(dot(N, V), 0.0f);
    const float NdotL   = max(dot(N, L), 0.0f);
    const float ggx1    = geometry_schlick_GGX(NdotV, r);
    const float ggx2    = geometry_schlick_GGX(NdotL, r);
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
    const float G   = geometry_smith(N, V, L.direction, M.roughness);
    vec3 F          = frensel_schlick(H, V, F0);

    vec3 kd = vec3(1.0f) - F;
    kd *= 1.0f - M.metallic;

    vec3 numerator      = NDF * F * G;
    const float denom   = 4.0f * max(dot(V, N), 0.0f) * max(dot(L.direction, N), 0.0f);
    vec3 specular       = numerator / max(denom, 0.001f);

    float NdotL = max(dot(N, L.direction), 0.0f);
    return (kd * (M.albedo / M_PI) + specular) * radiance * NdotL;
}

float shadow_value(vec4 light_space_pos, int kernel_radius, float penumbra_size, vec2 jitter_scale, float inv_shadow_map_size, float NdotL)
{
    const float pcf_matrix_sqrt     = 2 * kernel_radius + 1;
    const float shadow_contribution = 1.0f / (pcf_matrix_sqrt * pcf_matrix_sqrt);
    const float offset_scale        = inv_shadow_map_size * penumbra_size;   // inv_shadow_map_size * x, where x defines the size of the penumbra
    const float early_bail_offset   = ((1 + kernel_radius) * penumbra_size) * inv_shadow_map_size;

    // coordinate for to sample to jitter map
    vec3 jcoord = vec3(gl_FragCoord.xy * jitter_scale, 0.0f);

    // perspective devide of light space position
    // x and y store the normalized screen coordinates [-1.0, +1.0]
    // z stores the depth in the view of the light source
    vec3 light_project_coords = light_space_pos.xyz / light_space_pos.w; 
    if(light_project_coords.z > 1.0f)
        return 1.0f;

    // texture coordinates interval is [0.0, 1.0]
    light_project_coords.xy = light_project_coords.xy * 0.5f + 0.5f;

    /* early bailing */
    float sum = 0.0f;
    // take the samples of the corners of the sample matrix
    sum += texture(directional_shadow_map, vec3(light_project_coords.xy + vec2(+early_bail_offset), light_project_coords.z));
    sum += texture(directional_shadow_map, vec3(light_project_coords.xy + vec2(-early_bail_offset, +early_bail_offset), light_project_coords.z));
    sum += texture(directional_shadow_map, vec3(light_project_coords.xy + vec2(+early_bail_offset, -early_bail_offset), light_project_coords.z));
    sum += texture(directional_shadow_map, vec3(light_project_coords.xy + vec2(-early_bail_offset), light_project_coords.z));

    // additional samples to minimize the shadow error
    // remove those 4 lines for slightly better performance but there will be more shadow-artefacts
#if 1
    sum += texture(directional_shadow_map, vec3(light_project_coords.xy + vec2(+early_bail_offset, 0.0f), light_project_coords.z));
    sum += texture(directional_shadow_map, vec3(light_project_coords.xy + vec2(-early_bail_offset, 0.0f), light_project_coords.z));
    sum += texture(directional_shadow_map, vec3(light_project_coords.xy + vec2(0.0f, +early_bail_offset), light_project_coords.z));
    sum += texture(directional_shadow_map, vec3(light_project_coords.xy + vec2(0.0f, -early_bail_offset), light_project_coords.z));
#endif

    // if fragment is not at the penumbra, we dont need any more samples
    // 0.0f ... completely in shadow
    // 4.0f ... completely in light
    if(sum == 8.0f || sum == 0.0f || NdotL < 0.0f)
        return (sum == 0.0f) ? 0.0f : 1.0f;

    float visibility = 0.0f;
    for(int i=-kernel_radius; i<=kernel_radius; i++)
    {
        for(int j=-kernel_radius; j<=kernel_radius; j++)
        {
            vec2 jitter_value = texture(jitter_map, jcoord).rg - 0.5f;
            vec2 offset = (vec2(i, j) + jitter_value) * offset_scale;
            visibility += texture(directional_shadow_map, vec3(light_project_coords.xy + offset, light_project_coords.z));
            jcoord.z += shadow_contribution;
        }
    }
    return visibility * shadow_contribution;
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

    float shadow = shadow_value(
        f_light_space_pos, 
        fragment_variables.kernel_radius, 
        fragment_variables.penumbra_size, 
        fragment_variables.jitter_scale, 
        fragment_variables.inv_shadow_map_size,
        NdotL
    );

    vec3 light_intensity = directional_light(dlu.lights[0], mtlu.materials[f_instanceID], view_vec, f_normal) * shadow;
    vec3 hdr_color = (light_intensity + mtlu.materials[f_instanceID].albedo) * tex_color.rgb;
    vec3 ldr_color = hdr_color / (hdr_color + vec3(1.0f));

    out_color = vec4(ldr_color, tex_color.a * mtlu.materials[f_instanceID].alpha);
#endif
}