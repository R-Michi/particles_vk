#include "particles.h"
#include "../random/random.h"

#include <string.h>

ParticleEngine::ParticleEngine(void)
{
    this->initialized = false;
}

ParticleEngine::ParticleEngine(const Config& config)
{
    this->init(config);
}

ParticleEngine::~ParticleEngine(void)
{
    this->stop();
}

void ParticleEngine::init(const Config& config)
{
    if (!this->initialized)
    {
        this->config.gravity        = config.gravity;
        this->config.ground_level   = config.ground_level;
        this->config.max_bf         = config.max_bf;
        this->config.min_bf         = config.min_bf;
        this->config.max_size       = config.max_size;
        this->config.min_size       = config.min_size;
        this->config.velocity_mean  = config.velocity_mean;
        this->config.velocity_sigma = config.velocity_sigma;
        this->config.renderer       = config.renderer;
        this->config.spawn_pos      = config.spawn_pos;
        this->config.min_ttl        = config.min_ttl;
        this->config.max_ttl        = config.max_ttl;
        this->particles.resize(config.n_particles);
        this->initialized = true;
    }
}

void ParticleEngine::engine_func(ParticleEngine* instance, std::chrono::milliseconds start_delay)
{
    using namespace __internal_random;
    using namespace std::chrono;

    std::this_thread::sleep_for(start_delay);
    time_point<high_resolution_clock> old_time = high_resolution_clock::now();

    std::vector<Particle>* particles = &instance->particles;
    particle_vertex_t* map = (particle_vertex_t*)instance->config.renderer->get_vertex_buffer().map(sizeof(particle_vertex_t) * instance->particles.size(), 0);

    while (instance->running)
    {
        nanoseconds deltatime = duration_cast<nanoseconds>(high_resolution_clock::now() - old_time);
        old_time = high_resolution_clock::now();

        for (size_t i = 0; i < particles->size(); i++)
        {
            if (!particles->at(i).alive())
            {
                glm::vec3 velocity;
                velocity.x = normal_dist(instance->config.velocity_mean.x, instance->config.velocity_sigma.x);
                velocity.y = normal_dist(instance->config.velocity_mean.y, instance->config.velocity_sigma.y);
                velocity.z = normal_dist(instance->config.velocity_mean.z, instance->config.velocity_sigma.z);

                glm::vec4 color;
                color.r = uniform_real_dist(0.0f, 1.0f);
                color.g = uniform_real_dist(0.0f, 1.0f);
                color.b = uniform_real_dist(0.0f, 1.0f);
                color.a = 1.0f;

                float size = uniform_real_dist(instance->config.min_size, instance->config.max_size);
                uint64_t ttl = uniform_uint64_dist(instance->config.min_ttl.count(), instance->config.max_ttl.count());

                particles->at(i).set(instance->config.spawn_pos, color, size, velocity, milliseconds(ttl));
            }
            else
            {
                float bounce_factor = uniform_real_dist(instance->config.min_bf, instance->config.max_bf);
                particles->at(i).update(deltatime, instance->config.ground_level, instance->config.gravity, bounce_factor);
            }

            const particle_vertex_t& vertex = particles->at(i).vertex();
            memcpy(map + i, &vertex, sizeof(particle_vertex_t));
        }
    }

    instance->config.renderer->get_vertex_buffer().unmap();
}

void ParticleEngine::start(std::chrono::milliseconds start_delay)
{
    if (this->initialized && !this->running)
    {
        this->running = true;
        this->engine_thread = std::thread(engine_func, this, start_delay);
    }
}

void ParticleEngine::stop(void)
{
    if (this->running)
    {
        this->running = false;
        this->engine_thread.join();
    }
}