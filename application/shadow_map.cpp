#include "../VulkanApp.h"

void ShadowMap::init_depth_attachment(VkPhysicalDevice physical_device, VkDevice device, uint32_t queue_fam_index, uint32_t resolution)
{
    this->depth_attachment.set_physical_device(physical_device);
    this->depth_attachment.set_device(device);
    this->depth_attachment.set_image_extent({ resolution, resolution });
    this->depth_attachment.set_image_format(ParticlesConstants::SHADOW_DEPTH_FORMAT);
    this->depth_attachment.set_image_queue_family_index(queue_fam_index);
    this->depth_attachment.set_image_samples(VK_SAMPLE_COUNT_1_BIT);
    this->depth_attachment.set_image_usage(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    this->depth_attachment.set_view_format(ParticlesConstants::SHADOW_DEPTH_FORMAT);
    this->depth_attachment.set_view_components({});
    this->depth_attachment.set_view_aspect_mask(VK_IMAGE_ASPECT_DEPTH_BIT);
    VULKAN_ASSERT(this->depth_attachment.create());
}

void ShadowMap::init_sampler(VkDevice device)
{
    VkSamplerCreateInfo sampler_create_info = {};
    sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_create_info.pNext = nullptr;
    sampler_create_info.flags = 0;
    sampler_create_info.magFilter = VK_FILTER_LINEAR;
    sampler_create_info.minFilter = VK_FILTER_LINEAR;
    sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler_create_info.mipLodBias = 0.0f;
    sampler_create_info.anisotropyEnable = VK_FALSE;
    sampler_create_info.maxAnisotropy = 1.0f;
    sampler_create_info.compareEnable = VK_TRUE;
    sampler_create_info.compareOp = VK_COMPARE_OP_LESS;
    sampler_create_info.minLod = 0.0f;
    sampler_create_info.maxLod = 0.0f;
    sampler_create_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    sampler_create_info.unnormalizedCoordinates = VK_FALSE;

    VULKAN_ASSERT(vkCreateSampler(device, &sampler_create_info, nullptr, &this->sampler));
}

void ShadowMap::init_render_pass(VkDevice device)
{
    VkAttachmentDescription attachment = {};
    attachment.flags = 0;
    attachment.format = ParticlesConstants::SHADOW_DEPTH_FORMAT;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference attachment_reference;
    attachment_reference.attachment = 0;
    attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription shadow_subpass = {};
    shadow_subpass.flags = 0;
    shadow_subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    shadow_subpass.inputAttachmentCount = 0;
    shadow_subpass.pInputAttachments = nullptr;
    shadow_subpass.colorAttachmentCount = 0;
    shadow_subpass.pColorAttachments = nullptr;
    shadow_subpass.pResolveAttachments = nullptr;
    shadow_subpass.pDepthStencilAttachment = &attachment_reference;
    shadow_subpass.preserveAttachmentCount = 0;
    shadow_subpass.pPreserveAttachments = nullptr;

    VkSubpassDependency shadow_dependency = {};
    shadow_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    shadow_dependency.dstSubpass = 0;
    shadow_dependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    shadow_dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    shadow_dependency.srcAccessMask = 0;
    shadow_dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    shadow_dependency.dependencyFlags = 0;

    VkRenderPassCreateInfo shadow_renderpass_create_info = {};
    shadow_renderpass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    shadow_renderpass_create_info.pNext = nullptr;
    shadow_renderpass_create_info.flags = 0;
    shadow_renderpass_create_info.attachmentCount = 1;
    shadow_renderpass_create_info.pAttachments = &attachment;
    shadow_renderpass_create_info.subpassCount = 1;
    shadow_renderpass_create_info.pSubpasses = &shadow_subpass;
    shadow_renderpass_create_info.dependencyCount = 1;
    shadow_renderpass_create_info.pDependencies = &shadow_dependency;

    VULKAN_ASSERT(vkCreateRenderPass(device, &shadow_renderpass_create_info, nullptr, &this->render_pass));
}

void ShadowMap::init_fbo(VkDevice device, uint32_t resolution)
{
    VkImageView shadow_attachment = this->depth_attachment.view();

    VkFramebufferCreateInfo fbo_create_info = {};
    fbo_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbo_create_info.pNext = nullptr;
    fbo_create_info.flags = 0;
    fbo_create_info.renderPass = this->render_pass;
    fbo_create_info.attachmentCount = 1;
    fbo_create_info.pAttachments = &shadow_attachment;
    fbo_create_info.width = resolution;
    fbo_create_info.height = resolution;
    fbo_create_info.layers = 1;

    VULKAN_ASSERT(vkCreateFramebuffer(device, &fbo_create_info, nullptr, &this->fbo));
}

void ShadowMap::init(VkPhysicalDevice physical_device, VkDevice device, uint32_t queue_fam_index, uint32_t resolution)
{
    this->init_depth_attachment(physical_device, device, queue_fam_index, resolution);
    this->init_sampler(device);
    this->init_render_pass(device);
    this->init_fbo(device, resolution);
}

void ShadowMap::reshape(VkPhysicalDevice physical_device, VkDevice device, uint32_t queue_fam_index, uint32_t resolution)
{
    this->depth_attachment.clear();
    this->init_depth_attachment(physical_device, device, queue_fam_index, resolution);

    vkDestroyFramebuffer(device, this->fbo, nullptr);
    this->init_fbo(device, resolution);
}

void ShadowMap::clear(VkDevice device)
{
    vkDestroyFramebuffer(device, this->fbo, nullptr);
    vkDestroyRenderPass(device, this->render_pass, nullptr);
    vkDestroySampler(device, this->sampler, nullptr);
    this->depth_attachment.clear();
}