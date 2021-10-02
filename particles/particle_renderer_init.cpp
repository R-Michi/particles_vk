#include "particle_renderer.h"
#include <stb/stb_image.h>
#include <array>
#include <stdexcept>
#include <string>

using namespace particles;

ParticleRenderer::ParticleRenderer(void)
{
    this->_initialized = false;
    this->particle_buffer_map = nullptr;
    this->buffer_capacity = 0;
    this->transformation_matrices = nullptr;
    this->indirect_command = nullptr;
}

ParticleRenderer::ParticleRenderer(const ParticleRendererInitInfo& info) : ParticleRenderer()
{
    this->init(info);
}

ParticleRenderer::~ParticleRenderer(void)
{
    this->_destruct();
}

VkResult ParticleRenderer::init_command_pool(const ParticleRendererInitInfo& info)
{
    if (!info.external_command_pool)
    {
        VkCommandPoolCreateInfo pool_ci = {};
        pool_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_ci.pNext = nullptr;
        pool_ci.flags = 0;
        pool_ci.queueFamilyIndex = info.queue_family_index;

        VkResult res = vkCreateCommandPool(info.device, &pool_ci, nullptr, &this->command_pool);
        if (res != VK_SUCCESS) return res;
    }
    else
    {
        this->command_pool = info.command_pool;
    }
    this->external_command_pool = info.external_command_pool;
    return VK_SUCCESS;
}

VkResult ParticleRenderer::init_command_buffer(const ParticleRendererInitInfo& info)
{
    VkCommandBufferAllocateInfo cbo_ai = {};
    cbo_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbo_ai.pNext = nullptr;
    cbo_ai.commandPool = this->command_pool;
    cbo_ai.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    cbo_ai.commandBufferCount = 1;

    return vkAllocateCommandBuffers(info.device, &cbo_ai, &this->command_buffer);
}

VkResult ParticleRenderer::init_particle_buffer(const ParticleRendererInitInfo& info)
{
    VkDeviceSize buffer_size = sizeof(particle_t) * info.buffer_capacity;
    this->buffer_capacity = info.buffer_capacity;

    this->particle_buffer.set_physical_device(info.physical_device);
    this->particle_buffer.set_device(info.device);
    this->particle_buffer.set_create_flags(0);
    this->particle_buffer.set_create_queue_families(&info.queue_family_index, 1);
    this->particle_buffer.set_create_sharing_mode(VK_SHARING_MODE_EXCLUSIVE);
    this->particle_buffer.set_create_usage(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    this->particle_buffer.set_create_size(buffer_size);
    // use DMA-cache for buffer location
    this->particle_buffer.set_memory_properties(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
    VkResult result = this->particle_buffer.create();
    if (result != VK_SUCCESS) return result;

    this->particle_buffer_map = (particle_t*)this->particle_buffer.map(buffer_size, 0);
    return VK_SUCCESS;
}

VkResult ParticleRenderer::init_uniform_buffer(const ParticleRendererInitInfo& info)
{
    this->tm_buffer.set_physical_device(info.physical_device);
    this->tm_buffer.set_device(info.device);
    this->tm_buffer.set_create_flags(0);
    this->tm_buffer.set_create_queue_families(&info.queue_family_index, 1);
    this->tm_buffer.set_create_sharing_mode(VK_SHARING_MODE_EXCLUSIVE);
    this->tm_buffer.set_create_usage(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    this->tm_buffer.set_create_size(sizeof(TransformMatrices));
    // use DMA-cache for buffer location
    this->tm_buffer.set_memory_properties(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
    VkResult res = this->tm_buffer.create();
    if (res != VK_SUCCESS) return res;
    
    // leave the uniform buffer mapped for fast updating, it gets unmapped in ParticleRenderer::clear
    this->transformation_matrices = (TransformMatrices*)this->tm_buffer.map(sizeof(TransformMatrices), 0);
    return VK_SUCCESS;
}

VkResult ParticleRenderer::init_indirect_buffer(const ParticleRendererInitInfo& info)
{
    constexpr VkDeviceSize indirect_size = sizeof(VkDrawIndirectCommand); // there is always only one indirect command

    this->indirect_buffer.set_physical_device(info.physical_device);
    this->indirect_buffer.set_device(info.device);
    this->indirect_buffer.set_create_flags(0);
    this->indirect_buffer.set_create_queue_families(&info.queue_family_index, 1);
    this->indirect_buffer.set_create_sharing_mode(VK_SHARING_MODE_EXCLUSIVE);
    this->indirect_buffer.set_create_usage(VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
    this->indirect_buffer.set_create_size(indirect_size);
    // use DMA-cache for buffer location
    this->indirect_buffer.set_memory_properties(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
    VkResult result = this->indirect_buffer.create();
    if (result != VK_SUCCESS) return result;

    this->indirect_command = (VkDrawIndirectCommand*)this->indirect_buffer.map(indirect_size, 0);
    // initial values
    this->indirect_command->firstInstance = 0;
    this->indirect_command->instanceCount = 1;
    this->indirect_command->firstVertex = 0;
    this->indirect_command->vertexCount = 0;
    return VK_SUCCESS;
}

VkResult ParticleRenderer::load_textures(const ParticleRendererInitInfo& info)
{
    // load particle texture from file
    int w, h, c;
    uint8_t* data = stbi_load(info.particle_texture_path, &w, &h, &c, 4);   // force load 4 channels
    if (data == nullptr)
        throw std::runtime_error(std::string("ParticleRenderer could not load texture: ") + info.particle_texture_path);

    this->particle_texture.set_pyhsical_device(info.physical_device);
    this->particle_texture.set_device(info.device);
    this->particle_texture.set_command_pool(this->command_pool);
    this->particle_texture.set_queue(info.queue);
    this->particle_texture.set_image_flags(0);
    this->particle_texture.set_image_format(VK_FORMAT_R8G8B8A8_UNORM);
    this->particle_texture.set_image_extent({ static_cast<uint32_t>(w), static_cast<uint32_t>(h), 1 });
    this->particle_texture.set_image_array_layers(1);
    this->particle_texture.set_image_type(VK_IMAGE_TYPE_2D);
    this->particle_texture.set_image_queue_families(info.queue_family_index);
    
    // activate mipmapping now to get the amount of mip levels of the texture
    this->particle_texture.mipmap_generate(true);
    this->particle_texture.mipmap_filter(VK_FILTER_LINEAR);

    // mip levels are requiered by the subresource range
    VkImageSubresourceRange srr = {};
    srr.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // this will be ignored by the texture because this is set as standard
    srr.baseMipLevel = 0;
    srr.levelCount = this->particle_texture.mip_levels();   // only returns the mip levels if mipmapping is active
    srr.baseArrayLayer = 0;
    srr.layerCount = 1;

    this->particle_texture.set_view_components({});
    this->particle_texture.set_view_format(VK_FORMAT_R8G8B8A8_UNORM);
    this->particle_texture.set_view_type(VK_IMAGE_VIEW_TYPE_2D);
    this->particle_texture.set_view_subresource_range(srr);

    this->particle_texture.set_sampler_address_mode(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
    this->particle_texture.set_sampler_anisotropy_enable(VK_FALSE);
    this->particle_texture.set_sampler_max_anisotropy(0);
    this->particle_texture.set_sampler_border_color(VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK);
    this->particle_texture.set_sampler_compare_enable(VK_FALSE);
    this->particle_texture.set_sampler_compare_op(VK_COMPARE_OP_NEVER);
    this->particle_texture.set_sampler_lod(0.0f, static_cast<float>(this->particle_texture.mip_levels()));
    this->particle_texture.set_sampler_mag_filter(VK_FILTER_LINEAR);
    this->particle_texture.set_sampler_min_filter(VK_FILTER_LINEAR);
    this->particle_texture.set_sampler_mipmap_mode(VK_SAMPLER_MIPMAP_MODE_LINEAR);
    this->particle_texture.set_sampler_mip_lod_bias(0.0f);
    this->particle_texture.set_sampler_unnormalized_coordinates(VK_FALSE);

    return this->particle_texture.create(data, 4 * sizeof(uint8_t));
}

VkResult ParticleRenderer::init_descritpors(const ParticleRendererInitInfo& info)
{
    // descriptor set layout bindings
    std::array<VkDescriptorSetLayoutBinding, 2> bindings;
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

    // create descriptor set layouts
    VkDescriptorSetLayoutCreateInfo layout_ci = {};
    layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_ci.pNext = nullptr;
    layout_ci.flags = 0;
    layout_ci.bindingCount = bindings.size();
    layout_ci.pBindings = bindings.data();
    VkResult res = vkCreateDescriptorSetLayout(info.device, &layout_ci, nullptr, &this->descriptor_layout);
    if (res != VK_SUCCESS) return res;

    // pool sizes
    VkDescriptorPoolSize uniform_buffer_size = {};
    uniform_buffer_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniform_buffer_size.descriptorCount = 1;

    std::array<VkDescriptorPoolSize, 2> pool_sizes;
    pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_sizes[0].descriptorCount = 1;
    
    pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_sizes[1].descriptorCount = 1;

    // create descriptor pool
    VkDescriptorPoolCreateInfo pool_ci = {};
    pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_ci.pNext = nullptr;
    pool_ci.flags = 0;
    pool_ci.maxSets = 1;
    pool_ci.poolSizeCount = pool_sizes.size();
    pool_ci.pPoolSizes = pool_sizes.data();
    res = vkCreateDescriptorPool(info.device, &pool_ci, nullptr, &this->descriptor_pool);
    if (res != VK_SUCCESS) return res;

    // create descriptor sets
    VkDescriptorSetAllocateInfo set_ai = {};
    set_ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    set_ai.pNext = nullptr;
    set_ai.descriptorPool = this->descriptor_pool;
    set_ai.descriptorSetCount = 1;
    set_ai.pSetLayouts = &this->descriptor_layout;
    res = vkAllocateDescriptorSets(info.device, &set_ai, &this->descriptor_set);
    if (res != VK_SUCCESS) return res;

    // update descriptor sets
    std::array<VkDescriptorBufferInfo, 1> buffer_infos;
    buffer_infos[0].buffer = this->tm_buffer.handle();
    buffer_infos[0].offset = 0;
    buffer_infos[0].range = sizeof(TransformMatrices);

    std::array<VkDescriptorImageInfo, 1> image_infos;
    image_infos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_infos[0].imageView = this->particle_texture.view();
    image_infos[0].sampler = this->particle_texture.sampler();

    std::array<VkWriteDescriptorSet, 2> descriptor_writes;
    descriptor_writes[0] = {};
    descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_writes[0].pNext = nullptr;
    descriptor_writes[0].dstSet = this->descriptor_set;
    descriptor_writes[0].dstBinding = 0;
    descriptor_writes[0].dstArrayElement = 0;
    descriptor_writes[0].descriptorCount = 1;
    descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_writes[0].pImageInfo = nullptr;
    descriptor_writes[0].pBufferInfo = buffer_infos.data() + 0;
    descriptor_writes[0].pTexelBufferView = nullptr;

    descriptor_writes[1] = {};
    descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_writes[1].pNext = nullptr;
    descriptor_writes[1].dstSet = this->descriptor_set;
    descriptor_writes[1].dstBinding = 1;
    descriptor_writes[1].dstArrayElement = 0;
    descriptor_writes[1].descriptorCount = 1;
    descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_writes[1].pImageInfo = image_infos.data() + 0;
    descriptor_writes[1].pBufferInfo = nullptr;
    descriptor_writes[1].pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(info.device, descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);

    return VK_SUCCESS;
}

VkResult ParticleRenderer::init_pipeline(const ParticleRendererInitInfo& info)
{
    VkResult res = VK_SUCCESS;

    // load shaders
    vka::Shader vertex_shader(info.device), geometry_shader(info.device), fragment_shader(info.device);
    vertex_shader.load(info.vertex_shader_path, VK_SHADER_STAGE_VERTEX_BIT);
    geometry_shader.load(info.geometry_shader_path, VK_SHADER_STAGE_GEOMETRY_BIT);
    fragment_shader.load(info.fragment_shader_path, VK_SHADER_STAGE_FRAGMENT_BIT);

    vka::ShaderProgram program;
    program.attach(vertex_shader);
    program.attach(geometry_shader);
    program.attach(fragment_shader);

    // get vertex attributes
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;
    particle_t::get_binding_description(bindings);
    particle_t::get_attribute_description(attributes);

    // initialize pipeline
    VkPipelineVertexInputStateCreateInfo vertex_input_ci = {};
    vertex_input_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_ci.pNext = nullptr;
    vertex_input_ci.flags = 0;
    vertex_input_ci.vertexBindingDescriptionCount = bindings.size();
    vertex_input_ci.pVertexBindingDescriptions = bindings.data();
    vertex_input_ci.vertexAttributeDescriptionCount = attributes.size();
    vertex_input_ci.pVertexAttributeDescriptions = attributes.data();

    VkPipelineInputAssemblyStateCreateInfo input_assembly_ci = {};
    input_assembly_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_ci.pNext = nullptr;
    input_assembly_ci.flags = 0;
    input_assembly_ci.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    input_assembly_ci.primitiveRestartEnable = VK_FALSE;

    // viewport and scissor gets set as dynamic state
    VkViewport init_viewport = {};
    VkRect2D init_scissor = {};
    VkPipelineViewportStateCreateInfo viewport_state_ci = {};
    viewport_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_ci.pNext = nullptr;
    viewport_state_ci.flags = 0;
    viewport_state_ci.viewportCount = 1;
    viewport_state_ci.pViewports = &init_viewport;
    viewport_state_ci.scissorCount = 1;
    viewport_state_ci.pScissors = &init_scissor;

    VkPipelineRasterizationStateCreateInfo resterizer_ci = {};
    resterizer_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    resterizer_ci.pNext = nullptr;
    resterizer_ci.flags = 0;
    resterizer_ci.depthClampEnable = VK_FALSE;
    resterizer_ci.rasterizerDiscardEnable = VK_FALSE;
    resterizer_ci.polygonMode = VK_POLYGON_MODE_FILL;
    resterizer_ci.cullMode = VK_CULL_MODE_NONE;
    resterizer_ci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    resterizer_ci.depthBiasEnable = VK_FALSE;
    resterizer_ci.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisample_state_ci = {};
    multisample_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state_ci.pNext = nullptr;
    multisample_state_ci.flags = 0;
    multisample_state_ci.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample_state_ci.sampleShadingEnable = VK_FALSE;
    multisample_state_ci.minSampleShading = 1.0f;
    multisample_state_ci.pSampleMask = nullptr;
    multisample_state_ci.alphaToCoverageEnable = VK_FALSE;
    multisample_state_ci.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depth_state_ci = {};
    depth_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_state_ci.pNext = nullptr;
    depth_state_ci.flags = 0;
    depth_state_ci.depthTestEnable = VK_TRUE;
    depth_state_ci.depthWriteEnable = VK_TRUE;
    depth_state_ci.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_state_ci.depthBoundsTestEnable = VK_FALSE;
    depth_state_ci.stencilTestEnable = VK_FALSE;
    depth_state_ci.front = {};
    depth_state_ci.back = {};
    depth_state_ci.minDepthBounds = 0.0f;
    depth_state_ci.maxDepthBounds = 1.0f;

    VkPipelineColorBlendAttachmentState color_blend_attachment = {};
    color_blend_attachment.blendEnable = VK_TRUE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.colorWriteMask = 0x0000000F;

    VkPipelineColorBlendStateCreateInfo color_blend_state_ci = {};
    color_blend_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_ci.pNext = nullptr;
    color_blend_state_ci.flags = 0;
    color_blend_state_ci.logicOpEnable = VK_FALSE;
    color_blend_state_ci.logicOp = VK_LOGIC_OP_NO_OP;
    color_blend_state_ci.attachmentCount = 1;
    color_blend_state_ci.pAttachments = &color_blend_attachment;
    color_blend_state_ci.blendConstants[0] = 0.0f;
    color_blend_state_ci.blendConstants[1] = 0.0f;
    color_blend_state_ci.blendConstants[2] = 0.0f;
    color_blend_state_ci.blendConstants[3] = 0.0f;

    std::array<VkDynamicState, 2> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamic_state_ci = {};
    dynamic_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_ci.pNext = nullptr;
    dynamic_state_ci.flags = 0;
    dynamic_state_ci.dynamicStateCount = dynamic_states.size();
    dynamic_state_ci.pDynamicStates = dynamic_states.data();

    VkPipelineLayoutCreateInfo pipe_layout_ci = {};
    pipe_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipe_layout_ci.pNext = nullptr;
    pipe_layout_ci.flags = 0;
    pipe_layout_ci.setLayoutCount = 1;
    pipe_layout_ci.pSetLayouts = &this->descriptor_layout;
    pipe_layout_ci.pushConstantRangeCount = 0;
    pipe_layout_ci.pPushConstantRanges = nullptr;
    res = vkCreatePipelineLayout(info.device, &pipe_layout_ci, nullptr, &this->pipeline_layout);
    if (res != VK_SUCCESS) return res;

    VkGraphicsPipelineCreateInfo pipe_ci = {};
    pipe_ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipe_ci.pNext = nullptr;
    pipe_ci.flags = 0;
    pipe_ci.stageCount = program.count();
    pipe_ci.pStages = program.get_stages();
    pipe_ci.pVertexInputState = &vertex_input_ci;
    pipe_ci.pInputAssemblyState = &input_assembly_ci;
    pipe_ci.pTessellationState = nullptr;
    pipe_ci.pViewportState = &viewport_state_ci;
    pipe_ci.pRasterizationState = &resterizer_ci;
    pipe_ci.pMultisampleState = &multisample_state_ci;
    pipe_ci.pDepthStencilState = &depth_state_ci;
    pipe_ci.pColorBlendState = &color_blend_state_ci;
    pipe_ci.pDynamicState = &dynamic_state_ci;
    pipe_ci.layout = this->pipeline_layout;
    pipe_ci.renderPass = info.render_pass;
    pipe_ci.subpass = info.sub_pass;
    pipe_ci.basePipelineHandle = VK_NULL_HANDLE;
    pipe_ci.basePipelineIndex = -1;
    return vkCreateGraphicsPipelines(info.device, VK_NULL_HANDLE, 1, &pipe_ci, nullptr, &this->pipeline);
}

VkResult ParticleRenderer::init(const ParticleRendererInitInfo& info)
{
    // error handling
    VkResult result = VK_SUCCESS;
    if (!this->_initialized)
    {
        if (info.physical_device == VK_NULL_HANDLE)
            throw std::invalid_argument("Physical device of ParticleRenderer::init is a VK_NULL_HANDLE.");
        if (info.device == VK_NULL_HANDLE)
            throw std::invalid_argument("Device of ParticleRenderer::init is a VK_NULL_HANDLE.");
        if (info.render_pass == VK_NULL_HANDLE)
            throw std::invalid_argument("Render pass of ParticleRenderer::init is a VK_NULL_HANDLE.");
        if (info.external_command_pool && info.command_pool == VK_NULL_HANDLE)
            throw std::invalid_argument("ParticleRenderer should use an external command pool but command pool of ParticleRenderer::init is a VK_NULL_HANDLE.");

        if ((result = this->init_command_pool(info))    != VK_SUCCESS) return result;
        if ((result = this->init_command_buffer(info))  != VK_SUCCESS) return result;
        if ((result = this->init_particle_buffer(info)) != VK_SUCCESS) return result;
        if ((result = this->init_uniform_buffer(info))  != VK_SUCCESS) return result;
        if ((result = this->init_indirect_buffer(info)) != VK_SUCCESS) return result;
        if ((result = this->load_textures(info))        != VK_SUCCESS) return result;
        if ((result = this->init_descritpors(info))     != VK_SUCCESS) return result;
        if ((result = this->init_pipeline(info))        != VK_SUCCESS) return result;
        this->_initialized = true;
    }
    else
    {
        throw std::runtime_error("ParticleRender has already been initialized.");
    }
    return result;
}

void ParticleRenderer::clear(VkDevice device)
{
    if (this->_initialized)
    {
        this->_initialized = false;

        vkDestroyPipeline(device, this->pipeline, nullptr);
        vkDestroyPipelineLayout(device, this->pipeline_layout, nullptr);
        vkDestroyDescriptorPool(device, this->descriptor_pool, nullptr);
        vkDestroyDescriptorSetLayout(device, this->descriptor_layout, nullptr);
        this->particle_texture.clear();
        this->tm_buffer.unmap();
        this->tm_buffer.clear();
        this->particle_buffer.unmap();
        this->particle_buffer.clear();
        this->indirect_buffer.unmap();
        this->indirect_buffer.clear();
        vkFreeCommandBuffers(device, this->command_pool, 1, &this->command_buffer);
        if (!this->external_command_pool)
            vkDestroyCommandPool(device, this->command_pool, nullptr);

        this->buffer_capacity = 0;
        this->particle_buffer_map = nullptr;
        this->transformation_matrices = nullptr;
        this->indirect_command = nullptr;
    }
}

void ParticleRenderer::_destruct(void)
{
    if (this->_initialized)
        throw std::runtime_error("ParticleRender must be cleared before destructor gets called.");
}
