#pragma once

#include "particle_pool.h"
#include <thread>
#include <atomic>
#include <set>

namespace particles
{
    class ParticleEngine
    {
    private:
        std::thread engine_thread;
        std::atomic_bool running;
        static void engine_func(ParticleEngine* engine, void* param);

    protected:
        void start_base(void* param);
        void stop_base(void);
        bool base_running(void) const noexcept { return this->running; }

    public:
        ParticleEngine(void);
        virtual ~ParticleEngine(void);

        virtual void run(const std::atomic_bool& running, void* param) = 0;
    };

    class StaticParticleEngine : public ParticleEngine
    {

    private:
        ParticlePool* pool;
        std::set<particle_t*> particles;

    public:
        StaticParticleEngine(void);
        StaticParticleEngine(ParticlePool& pool);
        ~StaticParticleEngine(void);

        void init(ParticlePool& pool);

        void start(void);
        void stop(void);
        void run(const std::atomic_bool& running, void* param);

        uint64_t spawn(const particle_t& particle);
        void kill(uint64_t uid);
        void kill_all(void);

        uint32_t count(void) const noexcept { return this->particles.size(); }
        bool running(void) const noexcept   { return this->base_running(); }
    };
};