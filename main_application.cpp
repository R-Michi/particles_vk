#include "VulkanApp.h"
#include "random/random.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <random>
#include <cmath>
#include <algorithm>

#ifndef M_PI
    #define M_PI 3.14159265358979323846f
#endif

float sin_lookup[36000], cos_lookup[36000];

float fast_sin(float x)
{
    return sin_lookup[static_cast<size_t>((x * 18000.0f) / M_PI)];
}

float fast_cos(float x)
{
    return cos_lookup[static_cast<size_t>((x * 18000.0f) / M_PI)];
}

glm::vec3 vogeldisk_sample_3f(int sample_index, int samples_count, float phi)
{
    float theta = 2.4f * (float)sample_index + phi;
    float r = sqrt((float)sample_index + 0.5f) / sqrt((float)samples_count);
    glm::vec2 u = r * glm::vec2(cos(theta), sin(theta));
    return glm::vec3(u.x, u.y, cos(r * (M_PI / 2)));
}

glm::vec2 vogeldisk_sample_2f(int sample_index, int samples_count, float phi)
{
    float theta = 2.4f * (float)sample_index + phi;
    float r = sqrt((float)sample_index + 0.5f) / sqrt((float)samples_count);
    return r * glm::vec2(cos(theta), sin(theta));
}

float RadicalInverse_VdC(uint32_t bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return static_cast<float>(bits) * 2.3283064365386963e-10; // / 0x100000000
}

// ---------------------------------------------------------------------------

glm::vec3 ImportanceSampleGGX(glm::vec2 Xi, glm::vec3 N, float roughness)
{
    float a = roughness * roughness;

    float phi = 2.0 * M_PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    // from spherical coordinates to cartesian coordinates
    glm::vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    // from tangent-space vector to world-space sample vector
    glm::vec3 up = glm::normalize(abs(N.z) < 0.999 ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(1.0f, 0.0f, 0.0f));
    //glm::vec up = abs(N.z) < 0.999 ? glm::vec3(0.0, 0.0f, 1.0f) : glm::vec3(1.0f, 0.0f, 0.0);
    glm::vec3 tangent = glm::normalize(glm::cross(up, N));
    glm::vec3 bitangent = glm::cross(N, tangent);

    glm::vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return glm::normalize(sampleVec);
}

glm::vec3 light_sample(const glm::vec2& xi, const glm::vec3& l_direction, float l_distance, float l_radius)
{
    const float phi = xi.x * 2.0f * M_PI;
    const float theta = 0.5f * M_PI - sqrt(xi.y) * atan(l_radius / l_distance);

    const glm::vec3 dir(fast_cos(theta) * fast_cos(phi), fast_cos(theta) * fast_sin(phi), fast_sin(theta));
    glm::vec3 up = abs(l_direction.z) < 0.999 ? glm::vec3(0.0, 0.0f, 1.0f) : glm::vec3(1.0f, 0.0f, 0.0);

    glm::mat3 TBN;
    TBN[2] = glm::normalize(l_direction);
    TBN[0] = glm::normalize(glm::cross(up, TBN[2]));
    TBN[1] = glm::cross(TBN[2], TBN[0]);

    return TBN * dir;
}


float white_noise(const glm::vec2& x)
{
    return glm::fract(sin(glm::dot(x, glm::vec2(12.9898, 78.233))) * 43758.5453);
}

glm::vec3 blue_noise(const glm::u8vec3* pixels, uint32_t w, const glm::uvec2& p)
{
    const glm::u8vec3* pixel = pixels + (p.y * w + p.x);
    return glm::vec3(pixel->x / 255.0f, pixel->y / 255.0f, pixel->z / 255.0f);
}

float hash(const glm::uvec2& x)
{
    glm::uvec2 q = 1103515245U * glm::uvec2((x.x >> 1U) ^ x.y, (x.y >> 1U) ^ x.x);
    uint32_t n = 1103515245U * (q.x ^ (q.y >> 3U));
    return static_cast<float>(n) * (1.0 / static_cast<float>(0xffffffffU));
}

glm::vec2 r2_sequence(uint32_t i, float lamda)
{
    constexpr float phi = 1.324717957244746;
    constexpr float delta0 = 0.76f;
    constexpr float i0 = 0.700f;
    constexpr glm::vec2 alpha(1.0f / phi, 1.0f / phi / phi);

    glm::vec2 u = glm::vec2(hash(glm::uvec2(i, 0)), hash(glm::uvec2(i, 1))) - 0.5f;
    return glm::fract(alpha * (float)i + lamda * delta0 * sqrt(M_PI) / (4.0f * sqrt((float)i - i0)) * u);
}

glm::vec2 Hammersley(uint32_t i, uint32_t N)
{
    return glm::vec2((float)i / (float)N, RadicalInverse_VdC(i));
}

glm::vec2 halton(glm::vec2 s)
{
    constexpr glm::vec2 coprimes(2, 3);
    glm::vec2 a(1, 1), b(0, 0);
    int x = 0;
    while (s.x > 0.0 && s.y > 0.0)
    {
        ++x;
        a = a / coprimes;
        b += a * glm::mod(s, coprimes);
        s = glm::floor(s / coprimes);
    }
    return b;
}

glm::vec2 golden_ratio_sequence(uint32_t i, float lamda) 
{
    constexpr glm::vec2 p0(M_PI / 2.0, 0.5);
    const uint32_t a = i & 0x0000FFFF;
    const uint32_t b = (i & 0xFFFF0000) >> 16;
    const float jitter = (hash({ a, b }) - 0.5f) * lamda;
    return glm::fract((p0 + glm::vec2(i * 12664745, i * 9560333) / float(0x1000000)) + jitter);
}

template<typename _T>
void random_shuffle(std::vector<_T>& vec)
{
    srand(clock());
    _T* _begin = &vec[0];
    int s = vec.size();
    for (int i = 0; i < s; i++)
        std::swap<_T>(*(_begin + i), *(_begin + (rand() % s)));
}

void ParticlesApp::application_main(ParticlesApp* app)
{
    using namespace __internal_random;

    particles::ParticlePool pool(app->particle_renderer);
    particles::StaticParticleEngine engine(pool);
    engine.start();

    glm::vec3 normal = glm::normalize(glm::vec3(0.0f, 1.0f, 1.0f));
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    glm::vec3 tangent = glm::normalize(glm::cross(up, normal));
    glm::vec3 betangent = glm::cross(normal, tangent);

    glm::mat3 TBN(tangent, normal, betangent);

    particles::particle_t particle, particle2;
    particle.color = { 0.0f, 1.0f, 1.0f, 1.0f};
    particle.size = 0.1f;

    particle2 = particle;

    for (int i = 0; i < 1000; i++)
    {
        particle.pos = glm::normalize(glm::vec3(uniform_real_dist(-1.0f, 1.0f), uniform_real_dist(0.0f, 1.0), uniform_real_dist(-1.0f, 1.0f))) * 3.0f;
        particle2.pos = TBN * particle.pos;

        particle.pos += glm::vec3(0.0f, 5.0f, 5.0f);
        particle2.pos += glm::vec3(7.0f, 5.0f, 5.0f);

        engine.spawn(particle);
        engine.spawn(particle2);
    }

    while (!app->renderer_shutdown) std::this_thread::yield();
    engine.stop();
}