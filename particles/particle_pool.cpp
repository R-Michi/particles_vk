#include "particle_pool.h"
#include <stdexcept>
#include <algorithm> 
#include <sstream>

using namespace particles;

ParticlePool::ParticlePool(void)
{
    // set every member value to initial state
    this->_clear();
}

ParticlePool::ParticlePool(ParticleRenderer& renderer) : ParticlePool()
{
    // initialize partilcle pool
    this->init(renderer);
}

ParticlePool::~ParticlePool(void)
{
    this->clear();
}

void ParticlePool::init(ParticleRenderer& renderer)
{
    if (this->_initialized)
        throw std::runtime_error("ParticlePool has already been initialized.");
    if (!renderer.initialized())
        throw std::invalid_argument("ParticleRenderer must be initialized, requiered from ParticlePool::init.");

    this->particle_buffer = renderer.get_particle_buffer();
    this->particle_capacity = renderer.capacity();
    this->particle_count = 0;                   // at initialization there are no particles allocated...
    this->indirect_command = renderer.get_indirect_command();
    this->indirect_command->vertexCount = 0;    // and there should no particles be drawn
    this->clear_memory();

    this->_renderer_initialized = &renderer._initialized;
    this->_initialized = true;
}

void ParticlePool::_clear(void)
{
    this->_initialized = false;
    this->_renderer_initialized = nullptr;
    this->indirect_command = nullptr;
    this->particle_capacity = 0;
    this->particle_count = 0;
    this->particle_buffer = nullptr;
}

void ParticlePool::clear(void)
{
    if (this->_initialized)
    {
        this->indirect_command->vertexCount = 0;    // set vertex count to 0, so that no particles will be drawn if pool gets destroyed
        this->_clear();
    }
}

void ParticlePool::clear_memory(void)
{
    // set every particle's position to NAN, as we need this for a shader-side check
    // and push every possible particle index to the heap, because nothing is allocated
    for (uint32_t i = 0; i < this->particle_capacity; i++)
    {
        (this->particle_buffer + i)->pos = glm::vec3(NAN);
        this->particle_heap.push_back(i);
    }
    std::make_heap(this->particle_heap.begin(), this->particle_heap.end(), std::greater<>{});
    this->allocated_particles.clear();
}

void ParticlePool::push_heap(uint32_t idx)
{
    // add the freed index to the particle heap
    this->particle_heap.push_back(idx);
    std::push_heap(this->particle_heap.begin(), this->particle_heap.end(), std::greater<>{});

    // remove the freed index from the set of allocated indices
    this->allocated_particles.erase(idx);
}

uint32_t ParticlePool::pop_heap(void)
{
    // pop the allocated index from the particle heap
    uint32_t idx = this->particle_heap[0];
    std::pop_heap(this->particle_heap.begin(), this->particle_heap.end(), std::greater<>{});
    this->particle_heap.pop_back();

    // add the allocated index to the set of allocated indices
    this->allocated_particles.insert(idx);
    return idx;
}

particle_t* ParticlePool::allocate(void)
{
    if (!this->_initialized)
        throw std::runtime_error("Failed to allocate particle.\nParticlePool must be initialized in order to allocate particles.");
    if (!*this->_renderer_initialized)
        throw std::runtime_error("Failed to allocate particle.\nParticleRenderer must be a valid object in order to allocate particles.");

    // particle pool out of memory
    if (this->particle_heap.size() == 0)
        return nullptr;

    // pop the allocated index from heap
    uint32_t particle_index = this->pop_heap();

    // vertex count is the highest allocated index + 1, because the index starts at 0
    // A std::set is a sorted binary tree, where the smallest element is the first element
    // and the highest element is the last element, or the first element in reversed order.
    this->indirect_command->vertexCount = (*this->allocated_particles.rbegin()) + 1;
    ++this->particle_count;

    return this->particle_buffer + particle_index;   // particle address = particle memory base address + index
}

void ParticlePool::free(particle_t* p_particle)
{
    if (!this->_initialized)
        throw std::runtime_error("Failed to free particle.\nParticlePool must be initialized in order to free particles.");
    if (!*this->_renderer_initialized)
        throw std::runtime_error("Failed to free particle.\nParticleRenderer must be a valid object in order to free particles.");
    if (p_particle == nullptr) return;
    
    // range check, if the particle to free is not part of the pool
    if (p_particle < this->particle_buffer || p_particle >= (this->particle_buffer + this->particle_capacity))
    {
        std::stringstream ss;
        ss << "Address " << p_particle << " is out of buffer range of the particle pool " << this << "." << std::endl;
        ss << "Particle pool's buffer range: " << this->base_address() << " - " << this->last_address();
        throw std::out_of_range(ss.str());
    }

    // We need a shader-side check wether a particle is allocated or not.
    // We could simply use an additional parameter in the particle_t-struct
    // but this implementation should be as memory and runtime efficient as possible.
    // For that reason, we take the position and set it to NAN. This is just fine as nothing
    // can be drawn with a NAN-position.
    p_particle->pos = glm::vec3(NAN);

    // get index of the particle to free
    uint32_t particle_index = p_particle - this->particle_buffer;

    // if particle is not allocated, do nothing
    if (!this->is_allocated(p_particle)) return;

    // push freed index to the heap
    this->push_heap(particle_index);

    // vertex count is the highest allocated index + 1, because the index starts at 0
    // A std::set is a sorted binary tree, where the smallest element is the first element
    // and the highest element is the last element, or the first element in reversed order.
    this->indirect_command->vertexCount = (this->allocated_particles.size() > 0) ? (*this->allocated_particles.rbegin()) + 1 : 0;
    --this->particle_count;
}

bool ParticlePool::is_allocated(const particle_t* p_particle) const noexcept
{
    if (!this->_initialized || p_particle == nullptr) return false;

    uint32_t particle_index = p_particle - this->particle_buffer;    // get index of the particle
    return (this->allocated_particles.find(particle_index) != this->allocated_particles.end());
}