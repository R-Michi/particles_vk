#include "particle_engine.h"
#include <stdexcept>

using namespace particles;

StaticParticleEngine::StaticParticleEngine(void)
{
    this->pool = nullptr;
}

StaticParticleEngine::StaticParticleEngine(ParticlePool& pool)
{
    this->init(pool);
}

StaticParticleEngine::~StaticParticleEngine(void)
{
    this->stop();
}

void StaticParticleEngine::init(ParticlePool& pool)
{
    if (this->base_running())
        std::runtime_error("Cannot reinitialize running particle engine (StaticParticleEngine).");
    this->pool = &pool;
}

void StaticParticleEngine::start(void)
{
    if (this->pool == nullptr)
        std::runtime_error("Cannot start uninitialized patrticle engine (StaticParticleEngine).");
    this->start_base(this);
}

void StaticParticleEngine::stop(void)
{
    this->stop_base();  // we stop the engine thread first, that no particle updates are processed anymore
    this->kill_all();   // kill all particles that we have spawned
}

uint64_t StaticParticleEngine::spawn(const particle_t& particle)
{
    uint64_t uid = 0;
    if (this->base_running())
    {
        // allocate particle
        particle_t* p = this->pool->allocate();
        if (p == nullptr)
            throw std::bad_alloc();

        // initialize particle with data
        *p = particle;

        // save particle
        this->particles.insert(p);

        // the address is unique for each particle
        // for a simple solution we take the address of the particle
        uid = reinterpret_cast<uint64_t>(p);
    }
    return uid;
}

void StaticParticleEngine::kill(uint64_t uid)
{
    if (this->base_running())
    {
        // for accessing the right particle, we transform the uid back to the address
        particle_t* p = reinterpret_cast<particle_t*>(uid);
        
        // we also have to check if we own the particle
        auto iter = this->particles.find(p);
        if (iter == this->particles.end()) return;

        //deallocate and delete particle
        this->pool->free(p);
        this->particles.erase(iter);
    }
}

void StaticParticleEngine::kill_all(void)
{
    if (this->base_running())
    {
        for (auto iter = this->particles.begin(); iter != this->particles.end(); iter++)
            this->pool->free(*iter);
        this->particles.clear();
    }
}

void StaticParticleEngine::run(const std::atomic_bool& running, void* param)
{
    // do nothing
}