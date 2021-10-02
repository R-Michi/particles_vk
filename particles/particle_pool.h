#pragma once

#include "particle_types.h"
#include "particle_renderer.h"

#include <vulkan/vulkan.h>
#include <vector>
#include <set>

namespace particles
{
    /**
    *   @brief Manages allocattion and deallocation of particles.
    *          The ParticlePool needs a ParticleRenderer which provides the particle buffer.
    *          Additionally, a check is implemented if the ParticleRenderer is still valid. 
               If the ParticleRenderer is not valid any more for whatever reason,
    *          particles cannot be allocated or deallocated.
    */
    class ParticlePool
    {
    private:
        particle_t* particle_buffer;                // base address of buffer
        uint32_t particle_capacity;                 // maximum number of particles the particle-buffer can store
        uint32_t particle_count;                    // count of how many particles are allocated
        std::vector<uint32_t> particle_heap;        // heap where the free particle indices are stored
        std::set<uint32_t> allocated_particles;     // set where the allocated particle indices are stored
        VkDrawIndirectCommand* indirect_command;    // indirect command struct for vkCmdDrawIndirect

        bool _initialized;
        const bool* _renderer_initialized;

        /**
        *   @brief Marks every particle in the particle-buffer as free.
        *          This method only gets called at initialization.
        */
        void clear_memory(void);

        /**
        *   @brief Pushes a free particle index to the heap.
        *          Used in 'ParticlePool::free'.
        *   @param Free index to push.
        */
        void push_heap(uint32_t idx);

        /**
        *   @brief Pops an allocated index from the heap.
        *         Used in 'ParticlePool::allocate'.
        *   @return Allocated index that got popped.
        */
        uint32_t pop_heap(void);

        /** @brief Sets every internal (private) member object to initial state. */
        void _clear(void);

    public:
        /**
        *   @brief The defualt constructor does not initialize the ParticlePool.
        *          In order to initialize the ParticlePool 'ParticlePool:init' must be called.
        */
        ParticlePool(void);

        /**
        *   @brief Constructor that initializes the ParticlePool.
        *   @param renderer: The ParticleRenderer that the ParticlePool should use.
        */
        ParticlePool(ParticleRenderer& renderer);

        /** @brief Destroys the ParticlePool. */
        virtual ~ParticlePool(void);

        /**
        *   @brief Completely initailizes the ParticlePool.
        *   @param renderer: The ParticleRenderer that the ParticlePool should use.
        */
        void init(ParticleRenderer& renderer);

        /** @brief Completely deinitializes the ParticlePool. */
        void clear(void);

        /**
        *   @brief Allocates one particle.
        *   @return A pointer to the allocated particle.
        */
        particle_t* allocate(void);

        /**
        *   @brief Deallocates one particle.
        *   @param p_particle: A pointer to the particle that should be deallocated.
        */
        void free(particle_t* p_particle);

        /** @return The maximum number of particles the ParticlePool can allocate. */
        uint32_t capacity(void) const noexcept              { return this->particle_capacity; }

        /** @return The number of particles that are currently allocated. */
        uint32_t count(void) const noexcept                 { return this->particle_count; }

        /** @return A pointer to the first particle in the particle-buffer.  */
        const particle_t* base_address(void) const noexcept { return this->particle_buffer; }

        /** @return A pointer to the last particle in the particle-buffer. */
        const particle_t* last_address(void) const noexcept { return this->particle_buffer + this->particle_capacity - 1; }

        /**
        *   @brief Checks if a particle is allocated.
        *   @param p_particle: A pointer to the particle to check if it is allocated.
        *   @return 'true' if: 
                - @param p_particle is allocated
        *   @return 'false' if: 
                - @param p_particle is a nullptr
                - @param p_particle is not in the particle-buffer range of the ParticlePool
        *       - the ParticlePool is not initialized
        */
        bool is_allocated(const particle_t* p_particle) const noexcept;

        /** @return 'true' if the ParticlePool is initialized. */
        bool initialized(void) const noexcept   { return this->_initialized; }

        /** @return 'true' if no particle has been allocated. */
        bool empty(void) const noexcept         { return (this->particle_count == 0); }

        /** @return 'true' if the ParticlePool is out of memory. */
        bool full(void) const noexcept          { return (this->particle_heap.size() == 0); }
    };
}