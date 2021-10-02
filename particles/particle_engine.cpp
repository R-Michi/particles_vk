#include "particle_engine.h"

using namespace particles;

ParticleEngine::ParticleEngine(void)
{
    this->running = false;
}

ParticleEngine::~ParticleEngine(void)
{
    this->stop_base();
}

void ParticleEngine::start_base(void* param)
{
    if (!this->running)
    {
        this->running = true;
        this->engine_thread = std::thread(ParticleEngine::engine_func, this, param);
    }
}

void ParticleEngine::stop_base(void)
{
    if (this->running)
    {
        this->running = false;
        this->engine_thread.join();
    }
}

void ParticleEngine::engine_func(ParticleEngine* engine, void* param)
{
    engine->run(engine->running, param);
}