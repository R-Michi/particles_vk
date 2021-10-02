#pragma once

#include <vulkan/vulkan_absraction.h>
#include "particle_types.h"

namespace particles
{
    /**
    *   Class: ParticleRenderer
    *   @brief This calss provides the buffer where particles get stored, allocated and deallocated.
    *          It also sets up a rendering pipeline that contains three shader stages: vertex-, geometry- and fragment-shader,
    *          and provides a pre-recorded secondary command buffer which must be executed EXTERNALLY.
    *          The matrices view and projection can be set via setter-methods.
    *   NOTE: There are no default shaders for particle rendering, they are free programmable.
    *         However, the shaders must implement specific input layouts and uniforms.
    *         Templates for those shaders are in the directory "./particles/shader_templates".
    */
    class ParticleRenderer
    {
        friend class ParticlePool;
    private:

        // vulkan handles
        VkDescriptorPool descriptor_pool;
        VkDescriptorSetLayout descriptor_layout;
        VkDescriptorSet descriptor_set;

        bool external_command_pool;
        VkCommandPool command_pool;
        VkCommandBuffer command_buffer;

        VkPipelineLayout pipeline_layout;
        VkPipeline pipeline;

        vka::Buffer particle_buffer;
        particle_t* particle_buffer_map;

        vka::Texture particle_texture;
        vka::Buffer tm_buffer;
        vka::Buffer indirect_buffer;

        // other variables
        bool _initialized;
        uint32_t buffer_capacity;
        TransformMatrices* transformation_matrices;
        VkDrawIndirectCommand* indirect_command;

        // internal methods to initialize the vulkan objects
        VkResult init_command_pool(const ParticleRendererInitInfo& info);
        VkResult init_command_buffer(const ParticleRendererInitInfo& info);
        VkResult init_particle_buffer(const ParticleRendererInitInfo& info);
        VkResult init_uniform_buffer(const ParticleRendererInitInfo& info);
        VkResult init_indirect_buffer(const ParticleRendererInitInfo& info);
        VkResult load_textures(const ParticleRendererInitInfo& info);
        VkResult init_descritpors(const ParticleRendererInitInfo& info);
        VkResult init_pipeline(const ParticleRendererInitInfo& info);

        /**
        *   @brief This method cannot be accessed from outside. It is only used by the particle pool
        *   to get read/write access to the particle's buffer memory.
        *   @return The (base) pointer to the particle buffer.
        */
        particle_t* get_particle_buffer(void) noexcept { return this->particle_buffer_map; }

        /**
        *   @brief This method cannot be accessed from outside. It is used by the particle pool
        *   to control how many particles should be drawn by an idirect draw command. The particle
        *   pool has read/write access to the indirect draw command.
        *   @return The pointer to a SINGLE VkDrawIndirectCommand-struct.
        */
        VkDrawIndirectCommand* get_indirect_command(void) noexcept { return this->indirect_command; }

        /**
        *   @brief Destructs the object. If the ParticleRenderer is initialized while the destructor gets called,
        *   an exception will be thrown. The ParticleRenderer must be cleared explicitly (through the call of 'ParticleRenderer::clear'),
        *   because the renderer contains Vulkan-objects that will trigger the validation layer(s) and may cause undifined
        *   behaviour if they are not destroyed in correct order.
        *   NOTE: Correct order would for example be:
        *         - destroy other vulkan objects    <- That applies to this class
        *         - destroy vulkan device           <- Is not handled by this class
        *         - destroy vulkan instance         <- Is not handled by this class
        */
        void _destruct(void);

    public:
        /**
        *   @brief The default constructor does not initialize the ParticleRenderer.
        *   In order to initialize the renderer 'ParticleRenderer::init' must be called.
        */
        ParticleRenderer(void);

        /**
        *   @brief Constructor that initializes the particle renderer.
        *   @param info: Initialization information struct
        */
        ParticleRenderer(const ParticleRendererInitInfo& info);

        /**  @brief Destructor. What the destructor does, is described at the method 'ParticleRenderer::_destruct'. */
        virtual ~ParticleRenderer(void);

        /**
        *   @brief Completely initializes the ParticleRenderer.
        *   @param Initialization information struct.
        */
        VkResult init(const ParticleRendererInitInfo& info);

        /**
        *   @brief Completely deinitializes the ParticleRenderer.
        *   @param Vulkan-object of logical device, must be the same as that was used by 'ParticleRenderer::init'.
        */
        void clear(VkDevice device);

        /**
        *   @brief Records a secondary command buffer that must be executed EXTERNALLY by 'vkCmdExecuteCommands'.
        *   @param Record information struct
        */
        VkResult record(const ParticleRendererRecordInfo& info);

        /** @brief Sets the particle-shader's view matrix. */
        void set_view(const glm::mat4& v) noexcept;

        /** @brief Sets the particle-shader's projection matrix. */
        void set_projection(const glm::mat4& p) noexcept;

        /** @brief Sets the particle-shader's view and projection matrix. */
        void set_view_projection(const glm::mat4& v, const glm::mat4& p) noexcept;

        /** @return 'true' if ParticleRenderer is initialized. */
        bool initialized(void) const noexcept                               { return this->_initialized; };

        /** @return The maximum number of particles the particle-buffer can store. */
        uint32_t capacity(void) const noexcept                              { return this->buffer_capacity; }

        /** @return A recorded command buffer that can be executed by 'vkCmdExecuteCommands'. */
        VkCommandBuffer get_command_buffer(void) const noexcept             { return this->command_buffer; }
        const VkCommandBuffer* get_command_buffer_ptr(void) const noexcept  { return &this->command_buffer; }
    };
};