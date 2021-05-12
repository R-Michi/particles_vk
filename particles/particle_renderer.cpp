#include "particles.h"
#include <stb/stb_image.h>

ParticleRenderer::ParticleRenderer(void) noexcept
{
    this->initialized = false;
}

ParticleRenderer::~ParticleRenderer(void)
{
    this->_destruct();
}

void ParticleRenderer::create_pipeline(const Config& config)
{
    std::vector<VkVertexInputBindingDescription> bindings;
    particle_vertex_t::get_binding_description(bindings);
    std::vector<VkVertexInputAttributeDescription> attributes;
    particle_vertex_t::get_attribute_description(attributes);

    VkPipelineVertexInputStateCreateInfo inp_state = {};
    inp_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    inp_state.pNext = nullptr;
    inp_state.flags = 0;
    inp_state.vertexBindingDescriptionCount = bindings.size();
    inp_state.pVertexBindingDescriptions = bindings.data();
    inp_state.vertexAttributeDescriptionCount = attributes.size();
    inp_state.pVertexAttributeDescriptions = attributes.data();

    VkPipelineInputAssemblyStateCreateInfo inp_assembly = {};
    inp_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inp_assembly.pNext = nullptr;
    inp_assembly.flags = 0;
    inp_assembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    inp_assembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(config.scr_width);
    viewport.height = static_cast<float>(config.scr_height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = { config.scr_width, config.scr_height };

    VkPipelineViewportStateCreateInfo viewport_state = {};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.pNext = nullptr;
    viewport_state.flags = 0;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = &viewport;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer_state = {};
    rasterizer_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer_state.pNext = nullptr;
    rasterizer_state.flags = 0;
    rasterizer_state.depthClampEnable = VK_FALSE;
    rasterizer_state.rasterizerDiscardEnable = VK_FALSE;
    rasterizer_state.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer_state.cullMode = VK_CULL_MODE_NONE;
    rasterizer_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer_state.depthBiasEnable = VK_FALSE;
    rasterizer_state.depthBiasConstantFactor = 0.0f;
    rasterizer_state.depthBiasClamp = 0.0f;
    rasterizer_state.depthBiasSlopeFactor = 0.0f;
    rasterizer_state.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisample_state = {};
    multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state.pNext = nullptr;
    multisample_state.flags = 0;
    multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample_state.sampleShadingEnable = VK_FALSE;
    multisample_state.minSampleShading = 1.0f;
    multisample_state.pSampleMask = nullptr;
    multisample_state.alphaToCoverageEnable = VK_FALSE;
    multisample_state.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depth_state = {};
    depth_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_state.pNext = nullptr;
    depth_state.flags = 0;
    depth_state.depthTestEnable = VK_TRUE;
    depth_state.depthWriteEnable = VK_TRUE;
    depth_state.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_state.depthBoundsTestEnable = VK_FALSE;
    depth_state.stencilTestEnable = VK_FALSE;
    depth_state.front = {};
    depth_state.back = {};
    depth_state.minDepthBounds = 0.0f;
    depth_state.maxDepthBounds = 1.0f;

    VkPipelineColorBlendAttachmentState cb_attachment = {};
    cb_attachment.blendEnable = VK_TRUE;
    cb_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    cb_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    cb_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    cb_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    cb_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    cb_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    cb_attachment.colorWriteMask = 0x0000000F;

    VkPipelineColorBlendStateCreateInfo color_blend_state = {};
    color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state.pNext = nullptr;
    color_blend_state.flags = 0;
    color_blend_state.logicOpEnable = VK_FALSE;
    color_blend_state.logicOp = VK_LOGIC_OP_NO_OP;
    color_blend_state.attachmentCount = 1;
    color_blend_state.pAttachments = &cb_attachment;
    color_blend_state.blendConstants[0] = 0.0f;
    color_blend_state.blendConstants[1] = 0.0f;
    color_blend_state.blendConstants[2] = 0.0f;
    color_blend_state.blendConstants[3] = 0.0f;

    std::vector<VkDynamicState> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state = {};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.pNext = nullptr;
    dynamic_state.flags = 0;
    dynamic_state.dynamicStateCount = dynamic_states.size();
    dynamic_state.pDynamicStates = dynamic_states.data();

    VkPipelineLayoutCreateInfo pipe_layout = {};
    pipe_layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipe_layout.pNext = nullptr;
    pipe_layout.flags = 0;
    pipe_layout.setLayoutCount = this->descriptor_set_layouts.size();
    pipe_layout.pSetLayouts = this->descriptor_set_layouts.data();
    pipe_layout.pushConstantRangeCount = 0;
    pipe_layout.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(this->device, &pipe_layout, nullptr, &this->pipeline_layout) != VK_SUCCESS)
        throw std::runtime_error("Particle Renderer: Failed to create pipeline layout.");

    VkGraphicsPipelineCreateInfo pipeline_create_info = {};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_info.pNext = nullptr;
    pipeline_create_info.flags = 0;
    pipeline_create_info.stageCount = config.p_program->count();
    pipeline_create_info.pStages = config.p_program->get_stages();
    pipeline_create_info.pVertexInputState = &inp_state;
    pipeline_create_info.pInputAssemblyState = &inp_assembly;
    pipeline_create_info.pTessellationState = nullptr;
    pipeline_create_info.pViewportState = &viewport_state;
    pipeline_create_info.pRasterizationState = &rasterizer_state;
    pipeline_create_info.pMultisampleState = &multisample_state;
    pipeline_create_info.pDepthStencilState = &depth_state;
    pipeline_create_info.pColorBlendState = &color_blend_state;
    pipeline_create_info.pDynamicState = &dynamic_state;
    pipeline_create_info.layout = this->pipeline_layout;
    pipeline_create_info.renderPass = config.render_pass;
    pipeline_create_info.subpass = config.subpass;
    pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_create_info.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(this->device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &this->pipeline) != VK_SUCCESS)
        throw std::runtime_error("Particle renderer: Failed to create graphics pipeline.");
}

void ParticleRenderer::create_descriptor_set_layouts(void)
{
    VkDescriptorSetLayoutBinding bindings[2];
    bindings[0] = {};
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_GEOMETRY_BIT;
    bindings[0].pImmutableSamplers = nullptr;

    bindings[1] = {};
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[1].pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo descr_set_layout_crate_info = {};
    descr_set_layout_crate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descr_set_layout_crate_info.pNext = nullptr;
    descr_set_layout_crate_info.flags = 0;
    descr_set_layout_crate_info.bindingCount = 2;
    descr_set_layout_crate_info.pBindings = bindings;

    VkDescriptorSetLayout layout;
    if (vkCreateDescriptorSetLayout(this->device, &descr_set_layout_crate_info, nullptr, &layout) != VK_SUCCESS)
        throw std::runtime_error("Particle renderer: Failed to create descriptor set layouts.");
    this->descriptor_set_layouts.push_back(layout);
}

void ParticleRenderer::create_descriptor_pool(void)
{
    VkDescriptorPoolSize pool_sizes[2];
    pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_sizes[0].descriptorCount = 1;

    pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_sizes[1].descriptorCount = 1;

    VkDescriptorPoolCreateInfo descr_pool_create_info = {};
    descr_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descr_pool_create_info.pNext = nullptr;
    descr_pool_create_info.flags = 0;
    descr_pool_create_info.maxSets = 1;
    descr_pool_create_info.poolSizeCount = 2;
    descr_pool_create_info.pPoolSizes = pool_sizes;

    if (vkCreateDescriptorPool(this->device, &descr_pool_create_info, nullptr, &this->descriptor_pool) != VK_SUCCESS)
        throw std::runtime_error("Particle renderer: failed to create descriptor pool.");
}

void ParticleRenderer::create_desctiptor_sets(void)
{
    VkDescriptorSetAllocateInfo descr_set_alloc_info = {};
    descr_set_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descr_set_alloc_info.pNext = nullptr;
    descr_set_alloc_info.descriptorPool = this->descriptor_pool;
    descr_set_alloc_info.descriptorSetCount = this->descriptor_set_layouts.size();
    descr_set_alloc_info.pSetLayouts = this->descriptor_set_layouts.data();

    this->descriptor_sets.resize(this->descriptor_set_layouts.size());
    if (vkAllocateDescriptorSets(this->device, &descr_set_alloc_info, this->descriptor_sets.data()) != VK_SUCCESS)
        throw std::runtime_error("Particle renderer: Failed to create descriptor sets.");

    VkDescriptorBufferInfo buffer_info = {};
    buffer_info.buffer = this->uniform_buffer.handle();
    buffer_info.offset = 0;
    buffer_info.range = sizeof(ParticleRenderer::TransformMatrics);

    VkDescriptorImageInfo image_info = {};
    image_info.sampler = this->texture.sampler();
    image_info.imageView = this->texture.view();
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet write_sets[2];
    write_sets[0] = {};
    write_sets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_sets[0].pNext = nullptr;
    write_sets[0].dstSet = this->descriptor_sets[0];
    write_sets[0].dstBinding = 0;
    write_sets[0].dstArrayElement = 0;
    write_sets[0].descriptorCount = 1;
    write_sets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write_sets[0].pImageInfo = nullptr;
    write_sets[0].pBufferInfo = &buffer_info;
    write_sets[0].pTexelBufferView = nullptr;

    write_sets[1] = {};
    write_sets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_sets[1].pNext = nullptr;
    write_sets[1].dstSet = this->descriptor_sets[0];
    write_sets[1].dstBinding = 1;
    write_sets[1].dstArrayElement = 0;
    write_sets[1].descriptorCount = 1;
    write_sets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write_sets[1].pImageInfo = &image_info;
    write_sets[1].pBufferInfo = nullptr;
    write_sets[1].pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(this->device, 2, write_sets, 0, nullptr);
}

void ParticleRenderer::create_command_buffers(const Config& config)
{
    VkCommandPoolCreateInfo command_pool_create_info = {};
    command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_create_info.pNext = nullptr;
    command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_create_info.queueFamilyIndex = config.family_index;

    if (vkCreateCommandPool(this->device, &command_pool_create_info, nullptr, &this->command_pool) != VK_SUCCESS)
        throw std::runtime_error("Particle renderer: Failed to create command pool.");

    VkCommandBufferAllocateInfo command_buffer_alloc_info = {};
    command_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_alloc_info.pNext = nullptr;
    command_buffer_alloc_info.commandPool = this->command_pool;
    command_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    command_buffer_alloc_info.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(this->device, &command_buffer_alloc_info, &this->command_buffer) != VK_SUCCESS)
        throw std::runtime_error("Particle renderer: Failed to allocate command buffer.");
}

void ParticleRenderer::create_vertex_buffer(const Config& config)
{
    this->vertex_buffer.set_physical_device(this->physical_device);
    this->vertex_buffer.set_device(this->device);
    this->vertex_buffer.set_command_pool(this->command_pool);
    this->vertex_buffer.set_queue(config.graphics_queue);
    this->vertex_buffer.set_create_flags(0);
    this->vertex_buffer.set_create_size(sizeof(particle_vertex_t) * config.max_particles);
    this->vertex_buffer.set_create_usage(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    this->vertex_buffer.set_create_sharing_mode(VK_SHARING_MODE_EXCLUSIVE);
    this->vertex_buffer.set_create_queue_families(&config.family_index, 1);
    this->vertex_buffer.set_memory_properties(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
    
    if (this->vertex_buffer.create() != VK_SUCCESS)
        throw std::runtime_error("Particle renderer: Failed to create vertex buffer.");

    // nullify memory
    void* map = this->vertex_buffer.map(sizeof(particle_vertex_t) * config.max_particles, 0);
    memset(map, 0, sizeof(particle_vertex_t) * config.max_particles);
    this->vertex_buffer.unmap();
}

void ParticleRenderer::create_uniform_buffer(const Config& config)
{
    this->uniform_buffer.set_physical_device(this->physical_device);
    this->uniform_buffer.set_device(this->device);
    this->uniform_buffer.set_command_pool(this->command_pool);
    this->uniform_buffer.set_queue(config.graphics_queue);
    this->uniform_buffer.set_create_flags(0);
    this->uniform_buffer.set_create_size(sizeof(TransformMatrics)); // only one matrix needed
    this->uniform_buffer.set_create_usage(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    this->uniform_buffer.set_create_sharing_mode(VK_SHARING_MODE_EXCLUSIVE);
    this->uniform_buffer.set_create_queue_families(&config.family_index, 1);
    this->uniform_buffer.set_memory_properties(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);

    if (this->uniform_buffer.create() != VK_SUCCESS)
        throw std::runtime_error("Particle renderer: Failed to create uniform buffer.");

    // nullify memory
    void* map = this->uniform_buffer.map(sizeof(TransformMatrics), 0);
    memset(map, 0, sizeof(TransformMatrics));
    this->uniform_buffer.unmap();
}

void ParticleRenderer::create_texture(const Config& config)
{
    int w, h, c;
    uint8_t* data = stbi_load(config.texture_path.c_str(), &w, &h, &c, 4);  // force load 4 channels
    if (data == nullptr)
        throw std::invalid_argument(std::string("Particle renderer: Failed to load texture:") + config.texture_path);

    this->texture.set_pyhsical_device(this->physical_device);
    this->texture.set_device(this->device);
    this->texture.set_command_pool(this->command_pool);
    this->texture.set_queue(config.graphics_queue);

    this->texture.set_image_flags(0);
    this->texture.set_image_extent({ static_cast<uint32_t>(w), static_cast<uint32_t>(h), 1 });
    this->texture.set_image_array_layers(1);
    this->texture.set_image_format(VK_FORMAT_R8G8B8A8_UNORM);
    this->texture.set_image_type(VK_IMAGE_TYPE_2D);
    this->texture.set_image_queue_families(config.family_index);

    VkImageSubresourceRange subresource = {};
    subresource.baseMipLevel = 0;
    subresource.levelCount = this->texture.mip_levels();
    subresource.baseArrayLayer = 0;
    subresource.layerCount = 1;

    this->texture.set_view_format(VK_FORMAT_R8G8B8A8_UNORM);
    this->texture.set_view_type(VK_IMAGE_VIEW_TYPE_2D);
    this->texture.set_view_components({});
    this->texture.set_view_subresource_range(subresource);
    
    this->texture.set_sampler_address_mode(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    this->texture.set_sampler_anisotropy_enable(false);
    this->texture.set_sampler_max_anisotropy(0);
    this->texture.set_sampler_border_color(VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK);
    this->texture.set_sampler_compare_enable(VK_FALSE);
    this->texture.set_sampler_compare_op(VK_COMPARE_OP_NEVER);
    this->texture.set_sampler_lod(0.0f, static_cast<float>(this->texture.mip_levels()) - 1.0f);
    this->texture.set_sampler_mag_filter(VK_FILTER_LINEAR);
    this->texture.set_sampler_min_filter(VK_FILTER_LINEAR);
    this->texture.set_sampler_mipmap_mode(VK_SAMPLER_MIPMAP_MODE_LINEAR);
    this->texture.set_sampler_mip_lod_bias(0.0f);
    this->texture.set_sampler_unnormalized_coordinates(false);

    this->texture.mipmap_filter(VK_FILTER_LINEAR);
    this->texture.mipmap_generate(true);

    if (this->texture.create(data, sizeof(uint8_t) * 4) != VK_SUCCESS)
        throw std::runtime_error("Particle renderer: Failed to create texture.");

    stbi_image_free(data);
} 

void ParticleRenderer::init(const Config& config)
{
    if (!this->initialized)
    {
        this->physical_device = config.physical_device;
        this->device = config.device;

        this->create_descriptor_set_layouts();
        this->create_pipeline(config);
        this->create_command_buffers(config);
        this->create_vertex_buffer(config);
        this->create_uniform_buffer(config);
        this->create_texture(config);
        this->create_descriptor_pool();
        this->create_desctiptor_sets();

        this->initialized = true;
    }
}

void ParticleRenderer::clear(void)
{
    if (this->initialized)
    {
        this->texture.clear();
        this->uniform_buffer.clear();
        this->vertex_buffer.clear();
        vkFreeCommandBuffers(this->device, this->command_pool, 1, &this->command_buffer);
        vkDestroyCommandPool(this->device, this->command_pool, nullptr);
        vkDestroyDescriptorPool(this->device, this->descriptor_pool, nullptr);
        for (VkDescriptorSetLayout layout : this->descriptor_set_layouts)
            vkDestroyDescriptorSetLayout(this->device, layout, nullptr);
        vkDestroyPipeline(this->device, this->pipeline, nullptr);
        vkDestroyPipelineLayout(this->device, this->pipeline_layout, nullptr);
        this->initialized = false;
    }
}

void ParticleRenderer::record(const Config& config)
{
    VkCommandBufferInheritanceInfo inheritance_info = {};
    inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    inheritance_info.pNext = nullptr;
    inheritance_info.renderPass = config.render_pass;
    inheritance_info.subpass = config.subpass;
    inheritance_info.framebuffer = VK_NULL_HANDLE;
    inheritance_info.occlusionQueryEnable = VK_FALSE;
    inheritance_info.queryFlags = 0;
    inheritance_info.pipelineStatistics = 0;

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pNext = nullptr;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    begin_info.pInheritanceInfo = &inheritance_info;

    if (vkBeginCommandBuffer(this->command_buffer, &begin_info) != VK_SUCCESS)
        throw std::runtime_error("Particle renderer: Failed to begin command buffer recording.");

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(config.scr_width);
    viewport.height = static_cast<float>(config.scr_height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = { config.scr_width, config.scr_height };

    vkCmdBindPipeline(this->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline);
    vkCmdSetViewport(this->command_buffer, 0, 1, &viewport);
    vkCmdSetScissor(this->command_buffer, 0, 1, &scissor);

    VkBuffer buffers[1] = { this->vertex_buffer.handle() };
    VkDeviceSize offsets[1] = { 0 };
    vkCmdBindVertexBuffers(this->command_buffer, 0, 1, buffers + 0, offsets + 0);
    vkCmdBindDescriptorSets(this->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline_layout, 0, 1, this->descriptor_sets.data(), 0, nullptr);

    vkCmdDraw(this->command_buffer, static_cast<uint32_t>(config.max_particles), 1, 0, 0);

    if (vkEndCommandBuffer(this->command_buffer) != VK_SUCCESS)
        throw std::runtime_error("Particle renderer: Failed to end command buffer recording.");
}

void ParticleRenderer::_destruct(void)
{
    if (this->initialized)
        throw std::runtime_error("Particle renderer must have been cleared.");
}