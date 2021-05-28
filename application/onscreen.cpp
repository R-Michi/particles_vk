#include "../VulkanApp.h"

void OnscreenRenderPass::init_swapchain(VkDevice device, VkSurfaceKHR surface, uint32_t queue_fam_index, uint32_t width, uint32_t height)
{
    VkSwapchainKHR old_swapchain = this->swapchain;

    VkSwapchainCreateInfoKHR swapchain_create_info = {};
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.pNext = nullptr;
    swapchain_create_info.flags = 0;
    swapchain_create_info.surface = surface;
    swapchain_create_info.minImageCount = ParticlesConstants::SWAPCHAIN_IMAGES;
    swapchain_create_info.imageFormat = ParticlesConstants::SWAPCHAIN_FORMAT;
    swapchain_create_info.imageColorSpace = ParticlesConstants::SWAPCHAIN_COLOR_SPACE;
    swapchain_create_info.imageExtent = { width, height };
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = ParticlesConstants::SWAPCHAIN_IMAGE_USAGE;
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_create_info.queueFamilyIndexCount = 1;
    swapchain_create_info.pQueueFamilyIndices = &queue_fam_index;
    swapchain_create_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode = ParticlesConstants::PRESENTATION_MODE;
    swapchain_create_info.clipped = VK_TRUE;
    swapchain_create_info.oldSwapchain = old_swapchain;

    VkResult result = vka::setup_swapchain(device, swapchain_create_info, this->swapchain, this->views);
    VULKAN_ASSERT(result);

    vkDestroySwapchainKHR(device, old_swapchain, nullptr);
}

void OnscreenRenderPass::init_depth_attachment(VkPhysicalDevice physical_device, VkDevice device, uint32_t queue_fam_index, uint32_t width, uint32_t height)
{
    this->depth_attachment.set_image_format(ParticlesConstants::DEPTH_FORMAT);
    this->depth_attachment.set_image_extent({ width, height });
    this->depth_attachment.set_image_samples(VK_SAMPLE_COUNT_1_BIT);
    this->depth_attachment.set_image_usage(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    this->depth_attachment.set_image_queue_family_index(queue_fam_index);
    this->depth_attachment.set_view_format(ParticlesConstants::DEPTH_FORMAT);
    this->depth_attachment.set_view_components({});
    this->depth_attachment.set_view_aspect_mask(VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
    this->depth_attachment.set_physical_device(physical_device);
    this->depth_attachment.set_device(device);
    VULKAN_ASSERT(this->depth_attachment.create());
}

void OnscreenRenderPass::init_render_pass(VkDevice device)
{
    constexpr static size_t ATTACHMENT_COUNT = 2;

    VkAttachmentDescription attachment_descr[ATTACHMENT_COUNT];
    attachment_descr[0] = {};
    attachment_descr[0].flags = 0;
    attachment_descr[0].format = ParticlesConstants::SWAPCHAIN_FORMAT;
    attachment_descr[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachment_descr[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_descr[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_descr[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_descr[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_descr[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment_descr[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    attachment_descr[1] = {};
    attachment_descr[1].flags = 0;
    attachment_descr[1].format = ParticlesConstants::DEPTH_FORMAT;
    attachment_descr[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachment_descr[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_descr[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_descr[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_descr[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_descr[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment_descr[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference attachment_references[ATTACHMENT_COUNT];
    attachment_references[0].attachment = 0;
    attachment_references[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    attachment_references[1].attachment = 1;
    attachment_references[1].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.flags = 0;
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = nullptr;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = attachment_references + 0;
    subpass.pResolveAttachments = nullptr;
    subpass.pDepthStencilAttachment = attachment_references + 1;
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = nullptr;

    VkSubpassDependency subpass_dependency = {};
    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependency.dstSubpass = 0;
    subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpass_dependency.srcAccessMask = 0;
    subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    subpass_dependency.dependencyFlags = 0;

    VkRenderPassCreateInfo renderpass_create_info = {};
    renderpass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderpass_create_info.pNext = nullptr;
    renderpass_create_info.flags = 0;
    renderpass_create_info.attachmentCount = ATTACHMENT_COUNT;
    renderpass_create_info.pAttachments = attachment_descr;
    renderpass_create_info.subpassCount = 1;
    renderpass_create_info.pSubpasses = &subpass;
    renderpass_create_info.dependencyCount = 1;
    renderpass_create_info.pDependencies = &subpass_dependency;

    VULKAN_ASSERT(vkCreateRenderPass(device, &renderpass_create_info, nullptr, &this->render_pass));
}   

void OnscreenRenderPass::init_fbos(VkDevice device, uint32_t width, uint32_t height)
{
    for (size_t i = 0; i < this->views.size(); i++)
    {
        std::vector<VkImageView> attachments = {
            this->views[i],
            this->depth_attachment.view()
        };

        VkFramebufferCreateInfo fbo_create_info = {};
        fbo_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbo_create_info.pNext = nullptr;
        fbo_create_info.flags = 0;
        fbo_create_info.renderPass = this->render_pass;
        fbo_create_info.attachmentCount = attachments.size();
        fbo_create_info.pAttachments = attachments.data();
        fbo_create_info.width = width;
        fbo_create_info.height = height;
        fbo_create_info.layers = 1;

        VkFramebuffer fbo;
        VULKAN_ASSERT(vkCreateFramebuffer(device, &fbo_create_info, nullptr, &fbo));
        this->fbos.push_back(fbo);
    }
}

void OnscreenRenderPass::init(VkPhysicalDevice physical_device, VkDevice device, VkSurfaceKHR surface, uint32_t queue_fam_index, uint32_t width, uint32_t height)
{
    this->init_swapchain(device, surface, queue_fam_index, width, height);
    this->init_depth_attachment(physical_device, device, queue_fam_index, width, height);
    this->init_render_pass(device);
    this->init_fbos(device, width, height);
}

void OnscreenRenderPass::clear(VkDevice device)
{
    for (VkFramebuffer fbo : this->fbos)
        vkDestroyFramebuffer(device, fbo, nullptr);
    for (VkImageView view : this->views)
        vkDestroyImageView(device, view, nullptr);
    vkDestroyRenderPass(device, this->render_pass, nullptr);
    this->depth_attachment.clear();
    vkDestroySwapchainKHR(device, swapchain, nullptr);
}