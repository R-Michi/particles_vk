#include "particles.h"

Particle::Particle(void) noexcept : _velocity(0.0f)
{
    this->_vertex.pos =     { 0.0f, 0.0f, 0.0f };
    this->_vertex.color =   { 0.0f, 0.0f, 0.0f, 0.0f };
    this->_vertex.size =    0.0f;
    this->_alive = false;
}

Particle::Particle(const glm::vec3& pos, const glm::vec4& color, float size, const glm::vec3& velocity, particle_duration ttl) noexcept
{
    this->set(pos, color, size, velocity, ttl);
}

Particle::Particle(const Particle& other) noexcept
{
    *this = other;
}

Particle& Particle::operator= (const Particle& other) noexcept
{
    this->_vertex   = other._vertex;
    this->_velocity = other._velocity;
    return *this;
}

Particle::~Particle(void) noexcept
{
    // dtor
}

void Particle::update(std::chrono::nanoseconds dt, float ground_level, float gravity, float bounce_factor) noexcept
{
    if (!this->_alive) return;

    particle_duration time_since_start = std::chrono::duration_cast<particle_duration>(std::chrono::high_resolution_clock::now() - this->tp_start);
    if (time_since_start >= this->_ttl)
    {
        this->_alive = false;
        return;
    }

    // bounce
    if (this->_vertex.pos.y < ground_level)
    {
        float under_ground = ground_level - this->_vertex.pos.y;
        this->_vertex.pos.y = ground_level + under_ground;
        this->_velocity.y *= -bounce_factor;
    }

    // velocity decreases, v(t) = v(t) + dv(t) -> v(t) = v0 - g * t -> dv(t) = a(t) = -g * t -> v(t) = v(t) - g * dt
    float deltatime = static_cast<float>(dt.count()) / 1e9f;
    this->_velocity.y -= gravity * deltatime;

    // s(t) = v * t -> s(t) = s(t) + ds(t) -> ds(t) = v * dt -> s(t) = s(t) + v * dt
    this->_vertex.pos += this->_velocity * deltatime;
}

void Particle::set(const glm::vec3& pos, const glm::vec4& color, float size, const glm::vec3& velocity, particle_duration ttl) noexcept
{
    this->_vertex.pos   = pos;
    this->_vertex.color = color;
    this->_vertex.size  = size;
    this->_velocity     = velocity;
    this->_ttl          = ttl;
    this->_alive        = true;
    this->tp_start      = std::chrono::high_resolution_clock::now();
}