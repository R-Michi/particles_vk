#pragma once

#include <glm/glm.hpp>
#include <chrono>
#include <vector>
#include <thread>
#include <atomic>

#include <vulkan/vulkan_absraction.h>

struct particle_vertex_t
{
    glm::vec3 pos;
    glm::vec4 color;
    float size;

    static void get_binding_description(std::vector<VkVertexInputBindingDescription>& descr);
    static void get_attribute_description(std::vector<VkVertexInputAttributeDescription>& descr);
};

class Particle
{
    using particle_duration = std::chrono::milliseconds;
    using particle_time_point = std::chrono::high_resolution_clock::time_point;

private:
    particle_vertex_t _vertex;
    glm::vec3 _velocity;
    particle_duration _ttl;
    bool _alive;
    particle_time_point tp_start;

public:
    Particle(void) noexcept;
    explicit Particle(const glm::vec3& pos, const glm::vec4& color, float size, const glm::vec3& velocity, particle_duration ttl) noexcept;

    Particle(const Particle& other) noexcept;
    Particle& operator= (const Particle& other) noexcept;

    virtual ~Particle(void) noexcept;

    void update(std::chrono::nanoseconds dt, float ground_level, float gravity, float bounce_factor) noexcept;
    void set(const glm::vec3& pos, const glm::vec4& color, float size, const glm::vec3& velocity, particle_duration ttl) noexcept;

    const particle_vertex_t vertex(void) const noexcept     { return this->_vertex; }
    const glm::vec3& velocity(void) const noexcept          { return this->_velocity; }
    bool alive(void) const noexcept                         { return this->_alive; }
};

class ParticleRenderer
{
public:
    struct Config
    {
        VkPhysicalDevice            physical_device;
        VkDevice                    device;
        VkRenderPass                render_pass;
        uint32_t                    subpass;
        uint32_t                    family_index;
        VkQueue                     graphics_queue;
        const vka::ShaderProgram*   p_program;
        std::string                 texture_path;
        uint32_t                    scr_width;
        uint32_t                    scr_height;
        size_t                      max_particles;
    };

    struct TransformMatrics
    {
        glm::mat4 view;
        glm::mat4 projection;
    };

private:
    VkPhysicalDevice physical_device;
    VkDevice device;

    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;

    std::vector<VkDescriptorSetLayout> descriptor_set_layouts;
    std::vector<VkDescriptorSet> descriptor_sets;
    VkDescriptorPool descriptor_pool;

    VkCommandPool command_pool;
    VkCommandBuffer command_buffer;

    vka::Buffer vertex_buffer;
    vka::Buffer uniform_buffer;
    vka::Texture texture;

    bool initialized;

    void create_pipeline(const Config& config);
    void create_descriptor_set_layouts(void);
    void create_descriptor_pool(void);
    void create_desctiptor_sets(void);
    void create_command_buffers(const Config& config);
    void create_vertex_buffer(const Config& config);
    void create_uniform_buffer(const Config& config);
    void create_texture(const Config& config);

    void _destruct(void);

public:

    ParticleRenderer(void) noexcept;
    ParticleRenderer(const ParticleRenderer&) = delete;
    ParticleRenderer& operator= (const ParticleRenderer&) = delete;
    ParticleRenderer(ParticleRenderer&&) = delete;
    ParticleRenderer& operator= (ParticleRenderer&&) = delete;
    virtual ~ParticleRenderer(void);

    void init(const Config& config);
    void clear(void);
    void record(const Config& config);

    vka::Buffer& get_vertex_buffer(void) noexcept               { return this->vertex_buffer; }
    vka::Buffer& get_uniform_buffer(void) noexcept              { return this->uniform_buffer; }
    VkCommandBuffer get_command_buffer(void) const noexcept     { return this->command_buffer; }
};

class ParticleEngine
{
public:
    struct Config
    {
        size_t                      n_particles;
        ParticleRenderer*           renderer;
        glm::vec3                   spawn_pos;
        float                       gravity;
        float                       min_bf;
        float                       max_bf;
        glm::vec3                   velocity_mean;
        glm::vec3                   velocity_sigma;
        float                       min_size;
        float                       max_size;
        float                       ground_level;
        std::chrono::milliseconds   min_ttl;
        std::chrono::milliseconds   max_ttl;
    };

private:
    std::vector<Particle> particles;
    std::thread engine_thread;
    std::atomic_bool running;
    static void engine_func(ParticleEngine* instance, std::chrono::milliseconds start_delay);

    Config config;
    bool initialized;

public:
    ParticleEngine(void);
    explicit ParticleEngine(const Config& config);

    ParticleEngine(const ParticleEngine&) = delete;
    ParticleEngine& operator= (const ParticleEngine&) = delete;
    ParticleEngine(ParticleEngine&&) = delete;
    ParticleEngine& operator= (ParticleEngine&&) = delete;

    virtual ~ParticleEngine(void);

    void set_spawn_pos(const glm::vec3& pos) noexcept                                   { this->config.spawn_pos = pos; }
    void set_gravity(float gravity) noexcept                                            { this->config.gravity = gravity; }
    void set_bounce_factor(float min, float max) noexcept                               { this->config.min_bf = min; this->config.max_bf = max; }
    void set_velocity(const glm::vec3& mean, const glm::vec3& sigma) noexcept           { this->config.velocity_mean = mean; this->config.velocity_sigma = sigma; }
    void set_size(float min, float max) noexcept                                        { this->config.min_size = min; this->config.max_size = max; }
    void set_ground_level(float level) noexcept                                         { this->config.ground_level = level; }
    void set_ttl(std::chrono::milliseconds min, std::chrono::milliseconds max) noexcept { this->config.min_ttl = min; this->config.max_ttl = max; }

    void init(const Config& config);

    void start(std::chrono::milliseconds start_delay);
    void stop(void);
};