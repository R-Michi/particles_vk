#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <vector>

namespace particles
{
    // forward class declarations
    class ParticlePool;

    /**
    *   @brief Contains rendering data: position, color and size of the particle.
    *   @param pos: Has vertex shader input layout location 0
    *   @param color: Has vertex shader input layout location 1
    *   @param size: Has vertex shader input layout location 2
    */
    struct particle_t
    {
        glm::vec3 pos;
        glm::vec4 color;
        float size;

        /** @return The binding descriptions for the particle_t struct (call by reference). */
        static void get_binding_description(std::vector<VkVertexInputBindingDescription>&);

        /** @return The attribute descriptions for the particle_t struct (call by reference). */
        static void get_attribute_description(std::vector<VkVertexInputAttributeDescription>&);
    };

    /**
    *   @brief View-space transformation matrices for the particle shader.
    *   NOTE: There is a shader-side 16-byte alignment for 'mat4' objects.
    */
    struct TransformMatrices
    {
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 projection;
    };

    /**
    *   @brief Struct that is used as parameter to initialize the particles::ParticleRenderer-class.
    *   @param vertex_shader_path: Path to the particle-vertex-shader
    *   @param geometry_shader_path: Path to the particle-geometry-shader
    *   @param fragment_shader_path: Path to the particle-fragment-shader
    *   @param physical_device: Physical device that is used for GPU-operations
    *   @param device: Logical device that is used for GPU-operations
    *   @param render_pass: Render pass to draw the particles with
    *   @param sub_pass: Render sub pass index that draws the particles.
    *   @param external_command_pool: Boolean to determine if an external command pool should be used
    *   @param command_pool: External command pool object, is ignored if @param external_command_pool is set to 'false'
    *   @param queue_family_index: The queue family index the renderer should use
    *   @param queue: The queue that is used for internal copy / move operations
    *   @param buffer_capycity: The maximum capacity how many particles the buffer can contain
    */
    struct ParticleRendererInitInfo
    {
        const char*         vertex_shader_path;
        const char*         geometry_shader_path;
        const char*         fragment_shader_path;
        const char*         particle_texture_path;
        VkPhysicalDevice    physical_device;
        VkDevice            device;
        VkRenderPass        render_pass;
        uint32_t            sub_pass;
        bool                external_command_pool;
        VkCommandPool       command_pool;
        uint32_t            queue_family_index;
        VkQueue             queue;
        uint32_t            buffer_capacity;
    };

    /**
    *   @brief Struct that is used as parameter for the particles::ParticleRenderer::record-method.
    *   @param render_pass: The render pass that is used for command buffer recording.
    *   @param sub_pass: The sub render pass that is used for command buffer recording.
    *   @param framebuffer: The framebuffer in which the command buffer gets executed.
    *                       This parameter can also be VK_NULL_HANDLE, if the framebuffer is unknown in which
    *                       the command buffer gets executed at runtime.
    *   @param viewport: The viewport of the scene.
    *   @param scissor: Scissor of the scene.
    */
    struct ParticleRendererRecordInfo
    {
        VkRenderPass    render_pass;
        uint32_t        sub_pass;
        VkFramebuffer   framebuffer;
        VkViewport      viewport;
        VkRect2D        scissor;
    };
};