#define VALIDATION_LAYERS 1
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "VulkanApp.h"
#include <stdexcept>
#include <vector>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <stb/stb_image.h>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

void __key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
}

ParticlesApp::ParticlesApp(void)
{
    this->swapchain = VK_NULL_HANDLE;
}

ParticlesApp::~ParticlesApp(void)
{
    this->destroy_vulkan();
    this->destry_glfw();
}

// ------------------------ MODELS ------------------------

void ParticlesApp::load_models(void)
{
    this->load_floor();
    this->load_fountain();
}

void ParticlesApp::load_floor(void)
{
    this->floor_vertices = {
        {{-150.0f, 0.0f, -150.0f}, {0.0f,   0.0f},   {0.0f, 1.0f, 0.0f}},
        {{+150.0f, 0.0f, -150.0f}, {150.0f, 0.0f},   {0.0f, 1.0f, 0.0f}},
        {{+150.0f, 0.0f, +150.0f}, {150.0f, 150.0f}, {0.0f, 1.0f, 0.0f}},
        {{-150.0f, 0.0f, +150.0f}, {0.0f,   150.0f}, {0.0f, 1.0f, 0.0f}}
    };

    this->floor_indices = {
        3, 0, 1, 1, 2, 3
    };
}

void ParticlesApp::load_fountain(void)
{
    vka::Model323 fountain;
    fountain.load(MODEL_FOUNTAIN);
    fountain.combine(this->fountain_vertices, this->fountain_indices);
}

// ------------------------ GLFW ------------------------

void ParticlesApp::init_glfw(void)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    this->vmode = glfwGetVideoMode(monitor);

    this->width = this->vmode->width / 2;
    this->height = this->vmode->height / 2;
    this->posx = this->vmode->width / 4;
    this->posy = this->vmode->height / 4;

    this->window = glfwCreateWindow(this->width, this->height, "Particles vulkan", nullptr, nullptr);
    if (this->window == nullptr)
        throw std::runtime_error("Failed to create GLFW window!");
    glfwSetWindowPos(this->window, this->posx, this->posy);
    
    glfwSetCursorPos(window, this->vmode->width / 2 + this->vmode->width / 4, this->vmode->height / 2 + this->vmode->height / 4);      // set mousepos to the center
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);   // disable cursor

    glfwSetKeyCallback(this->window, __key_callback);
}

void ParticlesApp::destry_glfw(void)
{
    glfwDestroyWindow(this->window);
    glfwTerminate();
}

// ------------------------ VULKAN ------------------------

void ParticlesApp::init_vulkan(void)
{
    this->create_app_info();
    this->create_instance();
    this->create_surface();

    this->create_physical_device();
    this->create_queues();
    this->create_device();
    this->create_swapchain();

    this->create_depth_attachment();
    this->create_render_pass();

    this->create_desciptor_set_layouts();
    this->create_pipeline();

    this->create_framebuffers();
    this->create_command_pool();
    this->create_command_buffers();

    this->create_vertex_buffers();
    this->create_index_buffers();
    this->create_uniform_buffers();
    this->create_textures();

    this->create_desciptor_pool();
    this->create_descriptor_sets();

    this->create_semaphores();

    this->init_particles();
    
    this->record_commands();
}

void ParticlesApp::create_app_info(void)
{
    this->app_info = {};
    this->app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    this->app_info.pNext = nullptr;
    this->app_info.pApplicationName = "Particles vulkan";
    this->app_info.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
    this->app_info.pEngineName = "";
    this->app_info.engineVersion = 0;
    this->app_info.apiVersion = 0;
}

void ParticlesApp::create_instance(void)
{
    std::vector<const char*> layers;
#if VALIDATION_LAYERS
    layers.push_back("VK_LAYER_LUNARG_standard_validation");
    layers.push_back("VK_LAYER_KHRONOS_validation");
#endif

    std::vector<const char*> extensions;
    vka::get_requiered_glfw_extensions(extensions);
#if VALIDATION_LAYERS
    extensions.push_back("VK_EXT_debug_utils");
#endif

    if (!vka::are_instance_layers_supported(layers))
    {
        std::string str = "Some requiered instance layers are not supported!\nRequiered instance layers:\n";
        for (const char* l : layers)
            str += '\t' + std::string(l) + '\n';
        throw std::runtime_error(str);
    }

    if (!vka::are_instance_extensions_supported(extensions))
    {
        std::string str = "Some requiered instance extensions are not supported!\nRequiered instance extensions:\n";
        for (const char* e : extensions)
            str += '\t' + std::string(e) + '\n';
        throw std::runtime_error(str);
    }

    VkInstanceCreateInfo instance_create_info = {};
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pNext = nullptr;
    instance_create_info.flags = 0;
    instance_create_info.pApplicationInfo = &this->app_info;
    instance_create_info.enabledLayerCount = layers.size();
    instance_create_info.ppEnabledLayerNames = layers.data();
    instance_create_info.enabledExtensionCount = extensions.size();
    instance_create_info.ppEnabledExtensionNames = extensions.data();

    VkResult result = vkCreateInstance(&instance_create_info, nullptr, &this->instance);
    VULKAN_ASSERT(result);
}

void ParticlesApp::create_surface(void)
{
    VkResult result = glfwCreateWindowSurface(this->instance, this->window, nullptr, &this->surface);
    VULKAN_ASSERT(result);
}


void ParticlesApp::create_physical_device(void)
{
    std::vector<VkPhysicalDevice> physical_devices;
    vka::get_physical_devices(this->instance, physical_devices);
    std::vector<VkPhysicalDeviceProperties> properties;
    vka::get_physical_device_properties(physical_devices, properties);

    vka::PhysicalDeviceFilter filter = {};
    filter.sequence = "";
    filter.reqMemoryPropertyFlags = { VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT };
    filter.reqDeviceTypeHirachy = { VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU };
    filter.pSurface = &this->surface;
    filter.reqMinImageCount = SWAPCHAIN_IMAGES;
    filter.reqMaxImageCount = SWAPCHAIN_IMAGES;
    filter.reqSurfaceImageUsageFlags = SWAPCHAIN_IMAGE_USAGE;
    filter.reqSurfaceColorFormats = { SWAPCHAIN_FORMAT };
    filter.reqSurfaceColorSpaces = { SWAPCHAIN_COLOR_SPACE };
    filter.reqPresentModes = { VK_PRESENT_MODE_FIFO_KHR, VK_PRESENT_MODE_MAILBOX_KHR };
    filter.reqQueueFamilyFlags = { VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT };

    size_t device_index;
    vka::PhysicalDeviceError error = vka::find_matching_physical_device(physical_devices, 0, filter, device_index);
    if (error != vka::VKA_PYHSICAL_DEVICE_ERROR_NONE)
        throw std::runtime_error(vka::physical_device_strerror(error));

    this->physical_device = physical_devices[device_index];
    std::cout << "Physical device found: " << properties[device_index].deviceName << std::endl;
}

void ParticlesApp::create_queues(void)
{
    std::vector<VkQueueFamilyProperties> properties;
    vka::get_queue_family_properties(this->physical_device, properties);

    vka::QueueFamilyFilter filter = {};
    filter.reqQueueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT;
    filter.reqQueueCount = 1;

    size_t index;
    vka::QueueFamilyError error = vka::find_matching_queue_family(properties, 0, filter, vka::VKA_QUEUE_FAMILY_PRIORITY_OPTIMAL, index);
    if (error != vka::VKA_QUEUE_FAMILY_ERROR_NONE)
        throw std::runtime_error(vka::queue_family_strerror(error));
    this->graphics_queue_family_index = static_cast<uint32_t>(index);
}

void ParticlesApp::create_device(void)
{
    float priority = 1.0f;

    VkDeviceQueueCreateInfo queue_create_info = {};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.pNext = nullptr;
    queue_create_info.flags = 0;
    queue_create_info.queueFamilyIndex = this->graphics_queue_family_index;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &priority;

    std::vector<const char*> extensions;
    extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    if (!vka::are_device_extensions_supported(this->physical_device, extensions))
    {
        std::string str = "Some requiered device extensions are not supported!\nRequiered device extensions:\n";
        for (const char* e : extensions)
            str += '\t' + std::string(e) + '\n';
        throw std::runtime_error(str);
    }

    VkPhysicalDeviceFeatures features = {};
    features.geometryShader = VK_TRUE;
    features.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo device_create_info = {};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pNext = nullptr;
    device_create_info.flags = 0;
    device_create_info.queueCreateInfoCount = 1;
    device_create_info.pQueueCreateInfos = &queue_create_info;
    device_create_info.enabledLayerCount = 0;
    device_create_info.ppEnabledLayerNames = nullptr;
    device_create_info.enabledExtensionCount = extensions.size();
    device_create_info.ppEnabledExtensionNames = extensions.data();
    device_create_info.pEnabledFeatures = &features;

    VkResult result = vkCreateDevice(this->physical_device, &device_create_info, nullptr, &this->device);
    VULKAN_ASSERT(result);

    VkBool32 support = false;
    result = vkGetPhysicalDeviceSurfaceSupportKHR(this->physical_device, this->graphics_queue_family_index, this->surface, &support);
    VULKAN_ASSERT(result);

    if (!support)
        throw std::runtime_error("Surface not supported!");

    vkGetDeviceQueue(this->device, this->graphics_queue_family_index, 0, &this->graphics_queue);
}

void ParticlesApp::create_swapchain(void)
{
    VkSwapchainKHR old_swapchain = this->swapchain;

    VkSwapchainCreateInfoKHR swapchain_create_info = {};
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.pNext = nullptr;
    swapchain_create_info.flags = 0;
    swapchain_create_info.surface = this->surface;
    swapchain_create_info.minImageCount = SWAPCHAIN_IMAGES;
    swapchain_create_info.imageFormat = SWAPCHAIN_FORMAT;
    swapchain_create_info.imageColorSpace = SWAPCHAIN_COLOR_SPACE;
    swapchain_create_info.imageExtent = { static_cast<uint32_t>(this->width), static_cast<uint32_t>(this->height) };
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = SWAPCHAIN_IMAGE_USAGE;
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_create_info.queueFamilyIndexCount = 1;
    swapchain_create_info.pQueueFamilyIndices = &this->graphics_queue_family_index;
    swapchain_create_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode = PRESENTATION_MODE;
    swapchain_create_info.clipped = VK_TRUE;
    swapchain_create_info.oldSwapchain = old_swapchain;

    VkResult result = vka::setup_swapchain(this->device, swapchain_create_info, this->swapchain, this->swapchain_views);
    VULKAN_ASSERT(result);

    vkDestroySwapchainKHR(this->device, old_swapchain, nullptr);
}


void ParticlesApp::create_depth_attachment(void)
{
    this->depth_attachment.set_image_format(DEPTH_FORMAT);
    this->depth_attachment.set_image_extent({ static_cast<uint32_t>(this->width), static_cast<uint32_t>(this->height) });
    this->depth_attachment.set_image_samples(VK_SAMPLE_COUNT_1_BIT);
    this->depth_attachment.set_image_usage(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    this->depth_attachment.set_image_queue_family_index(this->graphics_queue_family_index);
    this->depth_attachment.set_view_format(DEPTH_FORMAT);
    this->depth_attachment.set_view_components({});
    this->depth_attachment.set_view_aspect_mask(VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
    this->depth_attachment.set_physical_device(this->physical_device);
    this->depth_attachment.set_device(this->device);
    VULKAN_ASSERT(this->depth_attachment.create());
}

void ParticlesApp::create_render_pass(void)
{
    constexpr static size_t ATTACHMENT_COUNT = 2;

    VkAttachmentDescription attachment_descr[ATTACHMENT_COUNT];
    attachment_descr[0] = {};
    attachment_descr[0].flags = 0;
    attachment_descr[0].format = SWAPCHAIN_FORMAT;
    attachment_descr[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachment_descr[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_descr[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_descr[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_descr[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_descr[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment_descr[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    attachment_descr[1] = {};
    attachment_descr[1].flags = 0;
    attachment_descr[1].format = DEPTH_FORMAT;
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

    VULKAN_ASSERT(vkCreateRenderPass(this->device, &renderpass_create_info, nullptr, &this->renderpass_main));
}


void ParticlesApp::create_desciptor_set_layouts(void)
{
    VkDescriptorSetLayoutBinding bindings[2];
    bindings[0] = {};
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    bindings[0].pImmutableSamplers = nullptr;

    bindings[1] = {};
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = 2;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[1].pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo descr_layout_create_info = {};
    descr_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descr_layout_create_info.pNext = nullptr;
    descr_layout_create_info.flags = 0;
    descr_layout_create_info.bindingCount = 2;
    descr_layout_create_info.pBindings = bindings;

    VkDescriptorSetLayout layout;
    VULKAN_ASSERT(vkCreateDescriptorSetLayout(this->device, &descr_layout_create_info, nullptr, &layout));

    this->descriptor_set_layouts.push_back(layout);
}

void ParticlesApp::create_pipeline(void)
{
    vka::Shader vertex(this->device), fragment(this->device);
    vertex.load(SHADER_STATIC_SCENE_VERTEX_PATH, VK_SHADER_STAGE_VERTEX_BIT);
    fragment.load(SHADER_STATIC_SCENE_FRAGMENT_PATH, VK_SHADER_STAGE_FRAGMENT_BIT);

    vka::ShaderProgram shader;
    shader.attach(vertex);
    shader.attach(fragment);
    
    VkVertexInputBindingDescription binding_decr = {};
    binding_decr.binding = 0;
    binding_decr.stride = sizeof(vka::vertex323_t);
    binding_decr.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attrib_descr[3];
    attrib_descr[0] = {};
    attrib_descr[0].location = 0;
    attrib_descr[0].binding = 0;
    attrib_descr[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrib_descr[0].offset = 0;

    attrib_descr[1] = {};
    attrib_descr[1].location = 1;
    attrib_descr[1].binding = 0;
    attrib_descr[1].format = VK_FORMAT_R32G32_SFLOAT;
    attrib_descr[1].offset = sizeof(glm::vec3);

    attrib_descr[2] = {};
    attrib_descr[2].location = 2;
    attrib_descr[2].binding = 0;
    attrib_descr[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrib_descr[2].offset = sizeof(glm::vec3) + sizeof(glm::vec2);

    VkPipelineVertexInputStateCreateInfo vertex_input_create_info = {};
    vertex_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_create_info.pNext = nullptr;
    vertex_input_create_info.flags = 0;
    vertex_input_create_info.vertexBindingDescriptionCount = 1;
    vertex_input_create_info.pVertexBindingDescriptions = &binding_decr;
    vertex_input_create_info.vertexAttributeDescriptionCount = 3;
    vertex_input_create_info.pVertexAttributeDescriptions = attrib_descr;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info = {};
    input_assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_create_info.pNext = nullptr;
    input_assembly_create_info.flags = 0;
    input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_create_info.primitiveRestartEnable = VK_FALSE;

    VkViewport view_port = {};
    view_port.x = 0;
    view_port.y = 0;
    view_port.width = static_cast<float>(this->width);
    view_port.height = static_cast<float>(this->height);
    view_port.minDepth = 0.0f;
    view_port.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = { static_cast<uint32_t>(this->width), static_cast<uint32_t>(this->height) };

    VkPipelineViewportStateCreateInfo viewport_create_info = {};
    viewport_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_create_info.pNext = nullptr;
    viewport_create_info.flags = 0;
    viewport_create_info.viewportCount = 1;
    viewport_create_info.pViewports = &view_port;
    viewport_create_info.scissorCount = 1;
    viewport_create_info.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer_create_info = {};
    rasterizer_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer_create_info.pNext = nullptr;
    rasterizer_create_info.flags = 0;
    rasterizer_create_info.depthClampEnable = VK_FALSE;
    rasterizer_create_info.rasterizerDiscardEnable = VK_FALSE;
    rasterizer_create_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer_create_info.cullMode = VK_CULL_MODE_NONE;
    rasterizer_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer_create_info.depthBiasEnable = VK_FALSE;
    rasterizer_create_info.depthBiasConstantFactor = 0.0f;
    rasterizer_create_info.depthBiasClamp = 0.0f;
    rasterizer_create_info.depthBiasSlopeFactor = 0.0f;
    rasterizer_create_info.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisample_create_info = {};
    multisample_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_create_info.pNext = nullptr;
    multisample_create_info.flags = 0;
    multisample_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample_create_info.sampleShadingEnable = VK_FALSE;
    multisample_create_info.minSampleShading = 1.0f;
    multisample_create_info.pSampleMask = nullptr;
    multisample_create_info.alphaToCoverageEnable = VK_FALSE;
    multisample_create_info.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthtest_create_info = {};
    depthtest_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthtest_create_info.pNext = nullptr;
    depthtest_create_info.flags = 0;
    depthtest_create_info.depthTestEnable = VK_TRUE;
    depthtest_create_info.depthWriteEnable = VK_TRUE;
    depthtest_create_info.depthCompareOp = VK_COMPARE_OP_LESS;
    depthtest_create_info.depthBoundsTestEnable = VK_FALSE;
    depthtest_create_info.stencilTestEnable = VK_FALSE;
    depthtest_create_info.front = {};
    depthtest_create_info.back = {};
    depthtest_create_info.minDepthBounds = 0.0f;
    depthtest_create_info.maxDepthBounds = 1.0f;

    VkPipelineColorBlendAttachmentState color_blend_attachment = {};
    color_blend_attachment.blendEnable = VK_TRUE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.colorWriteMask = 0x0000000F;

    VkPipelineColorBlendStateCreateInfo color_blend_create_info = {};
    color_blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_create_info.pNext = nullptr;
    color_blend_create_info.flags = 0;
    color_blend_create_info.logicOpEnable = VK_FALSE;
    color_blend_create_info.logicOp = VK_LOGIC_OP_NO_OP;
    color_blend_create_info.attachmentCount = 1;
    color_blend_create_info.pAttachments = &color_blend_attachment;
    color_blend_create_info.blendConstants[0] = 0.0f;
    color_blend_create_info.blendConstants[1] = 0.0f;
    color_blend_create_info.blendConstants[2] = 0.0f;
    color_blend_create_info.blendConstants[3] = 0.0f;

    VkDynamicState dynamic_states[2];
    dynamic_states[0] = VK_DYNAMIC_STATE_VIEWPORT;
    dynamic_states[1] = VK_DYNAMIC_STATE_SCISSOR;

    VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {};
    dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_create_info.pNext = nullptr;
    dynamic_state_create_info.flags = 0;
    dynamic_state_create_info.dynamicStateCount = 2;
    dynamic_state_create_info.pDynamicStates = dynamic_states;

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.pNext = nullptr;
    pipeline_layout_create_info.flags = 0;
    pipeline_layout_create_info.setLayoutCount = this->descriptor_set_layouts.size();
    pipeline_layout_create_info.pSetLayouts = this->descriptor_set_layouts.data();
    pipeline_layout_create_info.pushConstantRangeCount = 0;
    pipeline_layout_create_info.pPushConstantRanges = nullptr;

    VULKAN_ASSERT(vkCreatePipelineLayout(this->device, &pipeline_layout_create_info, nullptr, &this->pipeline_layout));

    VkGraphicsPipelineCreateInfo pipeline_create_info = {};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_info.pNext = nullptr;
    pipeline_create_info.flags = 0;
    pipeline_create_info.stageCount = shader.count();
    pipeline_create_info.pStages = shader.get_stages();
    pipeline_create_info.pVertexInputState = &vertex_input_create_info;
    pipeline_create_info.pInputAssemblyState = &input_assembly_create_info;
    pipeline_create_info.pTessellationState = nullptr;
    pipeline_create_info.pViewportState = &viewport_create_info;
    pipeline_create_info.pRasterizationState = &rasterizer_create_info;
    pipeline_create_info.pMultisampleState = &multisample_create_info;
    pipeline_create_info.pDepthStencilState = &depthtest_create_info;
    pipeline_create_info.pColorBlendState = &color_blend_create_info;
    pipeline_create_info.pDynamicState = &dynamic_state_create_info;
    pipeline_create_info.layout = this->pipeline_layout;
    pipeline_create_info.renderPass = this->renderpass_main;
    pipeline_create_info.subpass = 0;
    pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_create_info.basePipelineIndex = -1;

    VULKAN_ASSERT(vkCreateGraphicsPipelines(this->device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &this->pipeline));
}


void ParticlesApp::create_framebuffers(void)
{
    for (size_t i = 0; i < this->swapchain_views.size(); i++)
    {
        std::vector<VkImageView> attachments = {
            this->swapchain_views[i],
            this->depth_attachment.view()
        };

        VkFramebufferCreateInfo fbo_create_info = {};
        fbo_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbo_create_info.pNext = nullptr;
        fbo_create_info.flags = 0;
        fbo_create_info.renderPass = this->renderpass_main;
        fbo_create_info.attachmentCount = attachments.size();
        fbo_create_info.pAttachments = attachments.data();
        fbo_create_info.width = static_cast<uint32_t>(this->width);
        fbo_create_info.height = static_cast<uint32_t>(this->height);
        fbo_create_info.layers = 1;

        VkFramebuffer fbo;
        VULKAN_ASSERT(vkCreateFramebuffer(this->device, &fbo_create_info, nullptr, &fbo));
        this->swapchain_fbos.push_back(fbo);
    }
}

void ParticlesApp::create_command_pool(void)
{
    VkCommandPoolCreateInfo command_pool_create_info = {};
    command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_create_info.pNext = nullptr;
    command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_create_info.queueFamilyIndex = this->graphics_queue_family_index;

    VULKAN_ASSERT(vkCreateCommandPool(this->device, &command_pool_create_info, nullptr, &this->command_pool));
}

void ParticlesApp::create_command_buffers(void)
{
    this->primary_command_buffers.resize(SWAPCHAIN_IMAGES);

    VkCommandBufferAllocateInfo command_buffer_alloc_info = {};
    command_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_alloc_info.pNext = nullptr;
    command_buffer_alloc_info.commandPool = this->command_pool;
    command_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_alloc_info.commandBufferCount = this->primary_command_buffers.size();

    VULKAN_ASSERT(vkAllocateCommandBuffers(this->device, &command_buffer_alloc_info, this->primary_command_buffers.data()));

    VkCommandBufferAllocateInfo static_scene_cbo_alloc_info = {};
    static_scene_cbo_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    static_scene_cbo_alloc_info.pNext = nullptr;
    static_scene_cbo_alloc_info.commandPool = this->command_pool;
    static_scene_cbo_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    static_scene_cbo_alloc_info.commandBufferCount = 1;

    VULKAN_ASSERT(vkAllocateCommandBuffers(this->device, &static_scene_cbo_alloc_info, &this->static_scene_command_buffer));
}


void ParticlesApp::create_vertex_buffers(void)
{
    // floor
    VkDeviceSize floor_size = sizeof(vka::vertex323_t) * this->floor_vertices.size();

    vka::Buffer floor_staging_buffer(this->physical_device, this->device);
    floor_staging_buffer.set_create_flags(0);
    floor_staging_buffer.set_create_queue_families(&this->graphics_queue_family_index, 1);
    floor_staging_buffer.set_create_sharing_mode(VK_SHARING_MODE_EXCLUSIVE);
    floor_staging_buffer.set_create_size(floor_size);
    floor_staging_buffer.set_create_usage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    floor_staging_buffer.set_memory_properties(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    VULKAN_ASSERT(floor_staging_buffer.create());

    void* map = floor_staging_buffer.map(floor_size, 0);
    memcpy(map, this->floor_vertices.data(), floor_size);
    floor_staging_buffer.unmap();

    this->floor_vertex_buffer.set_create_usage(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    this->floor_vertex_buffer.set_memory_properties(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    this->floor_vertex_buffer.set_command_pool(this->command_pool);
    this->floor_vertex_buffer.set_queue(this->graphics_queue);
    this->floor_vertex_buffer = std::move(floor_staging_buffer);

    // fountain
    VkDeviceSize fountain_size = sizeof(vka::vertex323_t) * this->fountain_vertices.size();

    vka::Buffer fountain_staging_buffer(this->physical_device, this->device);
    fountain_staging_buffer.set_create_flags(0);
    fountain_staging_buffer.set_create_queue_families(&this->graphics_queue_family_index, 1);
    fountain_staging_buffer.set_create_sharing_mode(VK_SHARING_MODE_EXCLUSIVE);
    fountain_staging_buffer.set_create_size(fountain_size);
    fountain_staging_buffer.set_create_usage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    fountain_staging_buffer.set_memory_properties(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    VULKAN_ASSERT(fountain_staging_buffer.create());

    map = fountain_staging_buffer.map(fountain_size, 0);
    memcpy(map, this->fountain_vertices.data(), fountain_size);
    fountain_staging_buffer.unmap();

    this->fountain_vertex_buffer.set_create_usage(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    this->fountain_vertex_buffer.set_memory_properties(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    this->fountain_vertex_buffer.set_command_pool(this->command_pool);
    this->fountain_vertex_buffer.set_queue(this->graphics_queue);
    this->fountain_vertex_buffer = std::move(fountain_staging_buffer);
}

void ParticlesApp::create_index_buffers(void)
{
    // floor
    VkDeviceSize floor_indices_size = sizeof(uint32_t) * this->floor_indices.size();

    vka::Buffer floor_staging_buffer(this->physical_device, this->device);
    floor_staging_buffer.set_create_flags(0);
    floor_staging_buffer.set_create_queue_families(&this->graphics_queue_family_index, 1);
    floor_staging_buffer.set_create_sharing_mode(VK_SHARING_MODE_EXCLUSIVE);
    floor_staging_buffer.set_create_size(floor_indices_size);
    floor_staging_buffer.set_create_usage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    floor_staging_buffer.set_memory_properties(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    VULKAN_ASSERT(floor_staging_buffer.create());

    void* map = floor_staging_buffer.map(floor_indices_size, 0);
    memcpy(map, this->floor_indices.data(), floor_indices_size);
    floor_staging_buffer.unmap();

    this->floor_index_buffer.set_create_usage(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    this->floor_index_buffer.set_memory_properties(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    this->floor_index_buffer.set_command_pool(this->command_pool);
    this->floor_index_buffer.set_queue(this->graphics_queue);
    this->floor_index_buffer = std::move(floor_staging_buffer);

    // fountain
    VkDeviceSize fountain_indices_size = sizeof(uint32_t) * this->fountain_indices.size();

    vka::Buffer fountain_staging_buffer(this->physical_device, this->device);
    fountain_staging_buffer.set_create_flags(0);
    fountain_staging_buffer.set_create_queue_families(&this->graphics_queue_family_index, 1);
    fountain_staging_buffer.set_create_sharing_mode(VK_SHARING_MODE_EXCLUSIVE);
    fountain_staging_buffer.set_create_size(fountain_indices_size);
    fountain_staging_buffer.set_create_usage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    fountain_staging_buffer.set_memory_properties(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    VULKAN_ASSERT(fountain_staging_buffer.create());

    map = fountain_staging_buffer.map(fountain_indices_size, 0);
    memcpy(map, this->fountain_indices.data(), fountain_indices_size);
    fountain_staging_buffer.unmap();

    this->fountain_index_buffer.set_create_usage(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    this->fountain_index_buffer.set_memory_properties(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    this->fountain_index_buffer.set_command_pool(this->command_pool);
    this->fountain_index_buffer.set_queue(this->graphics_queue);
    this->fountain_index_buffer = std::move(fountain_staging_buffer);
}

void ParticlesApp::create_uniform_buffers(void)
{
    this->tm_buffer.set_physical_device(this->physical_device);
    this->tm_buffer.set_device(this->device);
    this->tm_buffer.set_create_flags(0);
    this->tm_buffer.set_create_queue_families(&this->graphics_queue_family_index, 1);
    this->tm_buffer.set_create_sharing_mode(VK_SHARING_MODE_EXCLUSIVE);
    this->tm_buffer.set_create_size(sizeof(TransformMatrices));
    this->tm_buffer.set_create_usage(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    this->tm_buffer.set_memory_properties(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
    this->tm_buffer.create();

    void* map = this->tm_buffer.map(sizeof(TransformMatrices), 0);
    memset(map, 0, sizeof(TransformMatrices));
    this->tm_buffer.unmap();
}

void ParticlesApp::create_textures(void)
{
    // floor texture
    int w, h, c;
    uint8_t* data = stbi_load(TEXTURE_FLOOR, &w, &h, &c, 4);
    if (data == nullptr)
        std::runtime_error("Failed to load floor texture!");
    size_t px_stride = 4 * sizeof(uint8_t);

    VkImageSubresourceRange srr = {};
    srr.baseMipLevel = 0;
    srr.baseArrayLayer = 0;
    srr.layerCount = 1;

    this->floor_texture.set_image_flags(0);
    this->floor_texture.set_image_type(VK_IMAGE_TYPE_2D);
    this->floor_texture.set_image_extent({ static_cast<uint32_t>(w), static_cast<uint32_t>(h), 1 });
    this->floor_texture.set_image_array_layers(1);
    this->floor_texture.set_image_format(VK_FORMAT_R8G8B8A8_UNORM);
    this->floor_texture.set_image_queue_families(this->graphics_queue_family_index);
    this->floor_texture.set_view_components({});
    this->floor_texture.set_view_format(VK_FORMAT_R8G8B8A8_UNORM);
    this->floor_texture.set_view_type(VK_IMAGE_VIEW_TYPE_2D);
    this->floor_texture.set_view_subresource_range(srr);
    this->floor_texture.set_sampler_address_mode(VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    this->floor_texture.set_sampler_anisotropy_enable(true);
    this->floor_texture.set_sampler_max_anisotropy(16);
    this->floor_texture.set_sampler_border_color(VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK);
    this->floor_texture.set_sampler_compare_enable(false);
    this->floor_texture.set_sampler_compare_op(VK_COMPARE_OP_NEVER);
    this->floor_texture.set_sampler_lod(0.0f, this->floor_texture.mip_levels() - 1.0f);
    this->floor_texture.set_sampler_mip_lod_bias(0.0f);
    this->floor_texture.set_sampler_mag_filter(VK_FILTER_LINEAR);
    this->floor_texture.set_sampler_min_filter(VK_FILTER_LINEAR);
    this->floor_texture.set_sampler_mipmap_mode(VK_SAMPLER_MIPMAP_MODE_LINEAR);
    this->floor_texture.set_sampler_unnormalized_coordinates(false);
    this->floor_texture.mipmap_generate(true);
    this->floor_texture.mipmap_filter(VK_FILTER_LINEAR);
    this->floor_texture.set_pyhsical_device(this->physical_device);
    this->floor_texture.set_device(this->device);
    this->floor_texture.set_command_pool(this->command_pool);
    this->floor_texture.set_queue(this->graphics_queue);
    VULKAN_ASSERT(this->floor_texture.create(data, px_stride));
    stbi_image_free(data);

    // fountain texture
    data = stbi_load(TEXTURE_FOUNTAIN, &w, &h, &c, 4);
    if (data == nullptr)
        std::runtime_error("Failed to load fountain texture!");
    px_stride = 4 * sizeof(uint8_t);

    this->fountain_texture.set_image_flags(0);
    this->fountain_texture.set_image_type(VK_IMAGE_TYPE_2D);
    this->fountain_texture.set_image_extent({ static_cast<uint32_t>(w), static_cast<uint32_t>(h), 1 });
    this->fountain_texture.set_image_array_layers(1);
    this->fountain_texture.set_image_format(VK_FORMAT_R8G8B8A8_UNORM);
    this->fountain_texture.set_image_queue_families(this->graphics_queue_family_index);
    this->fountain_texture.set_view_components({});
    this->fountain_texture.set_view_format(VK_FORMAT_R8G8B8A8_UNORM);
    this->fountain_texture.set_view_type(VK_IMAGE_VIEW_TYPE_2D);
    this->fountain_texture.set_view_subresource_range(srr);
    this->fountain_texture.set_sampler_address_mode(VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    this->fountain_texture.set_sampler_anisotropy_enable(false);
    this->fountain_texture.set_sampler_max_anisotropy(0);
    this->fountain_texture.set_sampler_border_color(VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK);
    this->fountain_texture.set_sampler_compare_enable(false);
    this->fountain_texture.set_sampler_compare_op(VK_COMPARE_OP_NEVER);
    this->fountain_texture.set_sampler_lod(0.0f, this->floor_texture.mip_levels() - 1.0f);
    this->fountain_texture.set_sampler_mip_lod_bias(0.0f);
    this->fountain_texture.set_sampler_mag_filter(VK_FILTER_LINEAR);
    this->fountain_texture.set_sampler_min_filter(VK_FILTER_LINEAR);
    this->fountain_texture.set_sampler_mipmap_mode(VK_SAMPLER_MIPMAP_MODE_LINEAR);
    this->fountain_texture.set_sampler_unnormalized_coordinates(false);
    this->fountain_texture.mipmap_generate(true);
    this->fountain_texture.mipmap_filter(VK_FILTER_LINEAR);
    this->fountain_texture.set_pyhsical_device(this->physical_device);
    this->fountain_texture.set_device(this->device);
    this->fountain_texture.set_command_pool(this->command_pool);
    this->fountain_texture.set_queue(this->graphics_queue);
    VULKAN_ASSERT(this->fountain_texture.create(data, px_stride));
    stbi_image_free(data);
}


void ParticlesApp::create_desciptor_pool(void)
{
    VkDescriptorPoolSize pool_sizes[2];
    pool_sizes[0] = {};
    pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_sizes[0].descriptorCount = 1;

    pool_sizes[1] = {};
    pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_sizes[1].descriptorCount = 2;

    VkDescriptorPoolCreateInfo descr_pool_create_info = {};
    descr_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descr_pool_create_info.pNext = nullptr;
    descr_pool_create_info.flags = 0;
    descr_pool_create_info.maxSets = 1;
    descr_pool_create_info.poolSizeCount = 2;
    descr_pool_create_info.pPoolSizes = pool_sizes;

    VULKAN_ASSERT(vkCreateDescriptorPool(this->device, &descr_pool_create_info, nullptr, &this->descriptor_pool));
}

void ParticlesApp::create_descriptor_sets(void)
{
    VkDescriptorSetAllocateInfo descr_set_alloc_info = {};
    descr_set_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descr_set_alloc_info.pNext = nullptr;
    descr_set_alloc_info.descriptorPool = this->descriptor_pool;
    descr_set_alloc_info.descriptorSetCount = this->descriptor_set_layouts.size();
    descr_set_alloc_info.pSetLayouts = this->descriptor_set_layouts.data();

    this->descriptor_sets.resize(this->descriptor_set_layouts.size());
    VULKAN_ASSERT(vkAllocateDescriptorSets(this->device, &descr_set_alloc_info, this->descriptor_sets.data()));

    VkDescriptorBufferInfo buffer_info = {};
    buffer_info.buffer = this->tm_buffer.handle();
    buffer_info.offset = 0;
    buffer_info.range = sizeof(TransformMatrices);

    VkDescriptorImageInfo image_info[2];
    image_info[0] = {};
    image_info[0].sampler = this->floor_texture.sampler();
    image_info[0].imageView = this->floor_texture.view();
    image_info[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    image_info[1] = {};
    image_info[1].sampler = this->fountain_texture.sampler();
    image_info[1].imageView = this->fountain_texture.view();
    image_info[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet write_sets[2];
    write_sets[0] = {};
    write_sets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_sets[0].pNext = nullptr;
    write_sets[0].dstSet = this->descriptor_sets.at(0);
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
    write_sets[1].dstSet = this->descriptor_sets.at(0);
    write_sets[1].dstBinding = 1;
    write_sets[1].dstArrayElement = 0;
    write_sets[1].descriptorCount = 2;
    write_sets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write_sets[1].pImageInfo = image_info;
    write_sets[1].pBufferInfo = nullptr;
    write_sets[1].pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(this->device, 2, write_sets, 0, nullptr);
}


void ParticlesApp::create_semaphores(void)
{
    VkSemaphoreCreateInfo sem_create_info = {};
    sem_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    sem_create_info.pNext = nullptr;
    sem_create_info.flags = 0;
    
    VULKAN_ASSERT(vkCreateSemaphore(this->device, &sem_create_info, nullptr, &this->image_ready));
    VULKAN_ASSERT(vkCreateSemaphore(this->device, &sem_create_info, nullptr, &this->rendering_done));
}


void ParticlesApp::init_particles(void)
{
    constexpr static size_t n_particles = 10000000;

    vka::Shader particle_vert(this->device), particle_geom(this->device), particle_frag(this->device);
    particle_vert.load(PARTICLE_VERTEX_PATH, VK_SHADER_STAGE_VERTEX_BIT);
    particle_geom.load(PARTICLE_GEOMETRY_PATH, VK_SHADER_STAGE_GEOMETRY_BIT);
    particle_frag.load(PARTICLE_FRAGMENT_PATH, VK_SHADER_STAGE_FRAGMENT_BIT);

    vka::ShaderProgram program;
    program.attach(particle_vert);
    program.attach(particle_geom);
    program.attach(particle_frag);

    ParticleRenderer::Config particle_config = {};
    particle_config.physical_device = this->physical_device;
    particle_config.device = this->device;
    particle_config.render_pass = this->renderpass_main;
    particle_config.subpass = 0;
    particle_config.family_index = this->graphics_queue_family_index;
    particle_config.graphics_queue = this->graphics_queue;
    particle_config.p_program = &program;
    particle_config.texture_path = TEXTURE_PARTICLE;
    particle_config.scr_width = this->width;
    particle_config.scr_height = this->height;
    particle_config.max_particles = n_particles;

    this->particle_renderer.init(particle_config);
    this->particle_renderer.record(particle_config);

    ParticleEngine::Config engine_config = {};
    engine_config.n_particles = n_particles;
    engine_config.renderer = &this->particle_renderer;
    engine_config.spawn_pos = { 0.0f, 2.5f, 0.0f };
    engine_config.gravity = 9.81f;
    engine_config.min_bf = 0.7f;
    engine_config.max_bf = 0.9f;
    engine_config.velocity_mean = { 0.0f, 5.0f, 0.0f };
    engine_config.velocity_sigma = { 3.0f, 2.0f, 3.0f };
    engine_config.min_size = 0.15f;
    engine_config.max_size = 0.20f;
    engine_config.ground_level = 0.0f;
    engine_config.min_ttl = std::chrono::milliseconds(5000);
    engine_config.max_ttl = std::chrono::milliseconds(6000);

    this->particle_engine.init(engine_config);
    this->particle_engine.start(std::chrono::milliseconds(3000));
}

void ParticlesApp::record_commands(void)
{
    /* recording of particles's command buffer is in method "init_particles()" */
    this->record_static_scene();
    this->record_primary_commands();
}

void ParticlesApp::record_primary_commands(void)
{
    VkCommandBufferBeginInfo command_begin_info = {};
    command_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_begin_info.pNext = nullptr;
    command_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    command_begin_info.pInheritanceInfo = nullptr;

    for (size_t i = 0; i < this->swapchain_views.size(); i++)
    {
        VULKAN_ASSERT(vkBeginCommandBuffer(this->primary_command_buffers[i], &command_begin_info));

        VkRect2D render_area = {};
        render_area.offset = { 0, 0 };
        render_area.extent = { static_cast<uint32_t>(this->width), static_cast<uint32_t>(this->height) };

        VkClearValue clear_values[2];
        clear_values[0] = {};
        clear_values[0].color = { 0.0f, 0.0f, 0.0f, 0.0f };
        clear_values[1] = {};
        clear_values[1].depthStencil.depth = 1.0f;

        VkRenderPassBeginInfo render_pass_begin_info = {};
        render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_begin_info.pNext = nullptr;
        render_pass_begin_info.renderPass = this->renderpass_main;
        render_pass_begin_info.framebuffer = this->swapchain_fbos[i];
        render_pass_begin_info.renderArea = render_area;
        render_pass_begin_info.clearValueCount = 2;
        render_pass_begin_info.pClearValues = clear_values;

        vkCmdBeginRenderPass(this->primary_command_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

        // execute commands
        VkCommandBuffer particle_cbos[2] = { this->static_scene_command_buffer, this->particle_renderer.get_command_buffer() };
        vkCmdExecuteCommands(this->primary_command_buffers[i], 2, particle_cbos);

        vkCmdEndRenderPass(this->primary_command_buffers[i]);
        VULKAN_ASSERT(vkEndCommandBuffer(this->primary_command_buffers[i]));
    }
}

void ParticlesApp::record_static_scene(void)
{
    VkCommandBufferInheritanceInfo inheritance_info = {};
    inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    inheritance_info.pNext = nullptr;
    inheritance_info.renderPass = this->renderpass_main;
    inheritance_info.subpass = 0;
    inheritance_info.framebuffer = VK_NULL_HANDLE;
    inheritance_info.occlusionQueryEnable = VK_FALSE;
    inheritance_info.queryFlags = 0;
    inheritance_info.pipelineStatistics = 0;

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pNext = nullptr;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    begin_info.pInheritanceInfo = &inheritance_info;

    VULKAN_ASSERT(vkBeginCommandBuffer(this->static_scene_command_buffer, &begin_info));

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(this->width);
    viewport.height = static_cast<float>(this->height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = { static_cast<uint32_t>(this->width), static_cast<uint32_t>(this->height) };

    vkCmdBindPipeline(this->static_scene_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline);
    vkCmdSetViewport(this->static_scene_command_buffer, 0, 1, &viewport);
    vkCmdSetScissor(this->static_scene_command_buffer, 0, 1, &scissor);

    // draw floor
    VkBuffer buffers[2] = { this->floor_vertex_buffer.handle(), this->fountain_vertex_buffer.handle() };
    VkDeviceSize offsets[2] = { 0, 0 };
    vkCmdBindVertexBuffers(this->static_scene_command_buffer, 0, 1, buffers + 0, offsets + 0);
    vkCmdBindIndexBuffer(this->static_scene_command_buffer, this->floor_index_buffer.handle(), 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(this->static_scene_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline_layout, 0, 1, this->descriptor_sets.data(), 0, nullptr);

    vkCmdDrawIndexed(this->static_scene_command_buffer, this->floor_indices.size(), 1, 0, 0, 0);

    // draw fountain
    vkCmdBindVertexBuffers(this->static_scene_command_buffer, 0, 1, buffers + 1, offsets + 1);
    vkCmdBindIndexBuffer(this->static_scene_command_buffer, this->fountain_index_buffer.handle(), 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(this->static_scene_command_buffer, this->fountain_indices.size(), 1, 0, 0, 1);

    VULKAN_ASSERT(vkEndCommandBuffer(this->static_scene_command_buffer));
}


void ParticlesApp::draw_frame(void)
{
    uint32_t img_index;
    VULKAN_ASSERT(vkAcquireNextImageKHR(this->device, this->swapchain, ~(0UI64), this->image_ready, VK_NULL_HANDLE, &img_index));

    VkPipelineStageFlags stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo static_scene_submit_info = {};
    static_scene_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    static_scene_submit_info.pNext = nullptr;
    static_scene_submit_info.waitSemaphoreCount = 1;
    static_scene_submit_info.pWaitSemaphores = &this->image_ready;
    static_scene_submit_info.pWaitDstStageMask = &stage_mask;
    static_scene_submit_info.commandBufferCount = 1;
    static_scene_submit_info.pCommandBuffers = &this->primary_command_buffers[img_index];
    static_scene_submit_info.signalSemaphoreCount = 1;
    static_scene_submit_info.pSignalSemaphores = &this->rendering_done;

    VULKAN_ASSERT(vkQueueSubmit(this->graphics_queue, 1, &static_scene_submit_info, VK_NULL_HANDLE));

    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = nullptr;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &this->rendering_done;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &this->swapchain;
    present_info.pImageIndices = &img_index;
    present_info.pResults = nullptr;

    VULKAN_ASSERT(vkQueuePresentKHR(this->graphics_queue, &present_info));
}

void ParticlesApp::destroy_vulkan(void)
{
    vkDeviceWaitIdle(this->device);

    this->particle_engine.stop();
    this->particle_renderer.clear();

    vkDestroySemaphore(this->device, this->image_ready, nullptr);
    vkDestroySemaphore(this->device, this->rendering_done, nullptr);

    for (VkDescriptorSetLayout layout : this->descriptor_set_layouts)
        vkDestroyDescriptorSetLayout(this->device, layout, nullptr);
    vkDestroyDescriptorPool(this->device, this->descriptor_pool, nullptr);

    this->floor_texture.clear();
    this->floor_vertex_buffer.clear();
    this->floor_index_buffer.clear();
    this->fountain_vertex_buffer.clear();
    this->fountain_index_buffer.clear();
    this->fountain_texture.clear();
    this->tm_buffer.clear();

    vkFreeCommandBuffers(this->device, this->command_pool, this->primary_command_buffers.size(), this->primary_command_buffers.data());
    vkDestroyCommandPool(this->device, this->command_pool, nullptr);

    for (VkFramebuffer fbo : this->swapchain_fbos)
        vkDestroyFramebuffer(this->device, fbo, nullptr);

    vkDestroyPipeline(this->device, this->pipeline, nullptr);
    vkDestroyPipelineLayout(this->device, this->pipeline_layout, nullptr);

    vkDestroyRenderPass(this->device, this->renderpass_main, nullptr);
    this->depth_attachment.clear();

    for (VkImageView view : this->swapchain_views)
        vkDestroyImageView(this->device, view, nullptr);
    vkDestroySwapchainKHR(this->device, this->swapchain, nullptr);

    vkDestroyDevice(this->device, nullptr);
    vkDestroySurfaceKHR(this->instance, this->surface, nullptr);
    vkDestroyInstance(this->instance, nullptr);
}

// ------------------------ OTHERS ------------------------

glm::vec3 ParticlesApp::handle_move_keys(float speed, float yaw, const int keymap[6])
{
    glm::vec3 vel(0.0f);
    if (glfwGetKey(window, keymap[0]) == GLFW_PRESS)
    {
        vel.x += speed * sin(yaw);
        vel.z += speed * cos(yaw);
    }
    if (glfwGetKey(window, keymap[1]) == GLFW_PRESS)
    {
        vel.x += speed * sin(yaw + M_PI * 0.5);
        vel.z += speed * cos(yaw + M_PI * 0.5);
    }
    if (glfwGetKey(window, keymap[2]) == GLFW_PRESS)
    {
        vel.x += speed * sin(yaw + M_PI);
        vel.z += speed * cos(yaw + M_PI);
    }
    if (glfwGetKey(window, keymap[3]) == GLFW_PRESS)
    {
        vel.x += speed * sin(yaw + M_PI * 1.5);
        vel.z += speed * cos(yaw + M_PI * 1.5);
    }
    if (glfwGetKey(window, keymap[4]) == GLFW_PRESS)
        vel.y += speed;
    if (glfwGetKey(window, keymap[5]) == GLFW_PRESS)
        vel.y -= speed;
    return vel;
}

void ParticlesApp::mouse_action(GLFWwindow* window, int width, int height, double& yaw, double& pitch, float sensetivity)
{
    constexpr double roty_max = 89.9 / 180.0 * M_PI;
    const double width_half = width / 2;
    const double height_half = height / 2;

    double mx, my;
    glfwGetCursorPos(window, &mx, &my);

    const double deltax = width_half - mx;
    const double deltay = height_half - my;

    yaw -= deltax * sensetivity;
    pitch += deltay * sensetivity;

    if (yaw >= 2 * M_PI)
        yaw -= 2 * M_PI;
    else if (yaw <= -2 * M_PI)
        yaw += 2 * M_PI;

    if (pitch > roty_max)
        pitch = roty_max;
    else if (pitch < -roty_max)
        pitch = -roty_max;

    glfwSetCursorPos(window, width_half, height_half);
}

void ParticlesApp::move_action(GLFWwindow* window, glm::vec3& pos, const glm::vec3 velocity)
{
    static double old_time = glfwGetTime();
    double dt = glfwGetTime() - old_time;
    old_time = glfwGetTime();

    // s = v * t
    pos += velocity * static_cast<float>(dt);
}

void ParticlesApp::update_frame_contents(void)
{
    _config.cam.velocity = this->handle_move_keys(_config.movement_speed, _config.cam.yaw, MOVE_KEY_MAP);
    this->mouse_action(this->window, this->width, this->height, _config.cam.yaw, _config.cam.pitch, _config.sesitivity);
    this->move_action(this->window, _config.cam.pos, _config.cam.velocity);

    glm::mat4 model(1.0f);
    glm::mat4 view = glm::lookAt(glm::dvec3(_config.cam.pos.x, _config.cam.pos.y, _config.cam.pos.z),
                                 glm::dvec3(_config.cam.pos.x + sin(_config.cam.yaw) * cos(_config.cam.pitch),
                                            _config.cam.pos.y + sin(_config.cam.pitch),
                                            _config.cam.pos.z + cos(_config.cam.yaw) * cos(_config.cam.pitch)),
                                 glm::dvec3(0.0, -1.0, 0.0));
    glm::mat4 projection = glm::perspective(glm::radians(100.0f), static_cast<float>(this->width) / static_cast<float>(this->height), 0.001f, 100.0f);

    TransformMatrices tm;
    tm.MVP = projection * view * model;
    void* map = this->tm_buffer.map(sizeof(TransformMatrices), 0);
    memcpy(map, &tm, sizeof(TransformMatrices));
    this->tm_buffer.unmap();

    ParticleRenderer::TransformMatrics ptm;
    ptm.view = view;
    ptm.projection = projection;

    map = this->particle_renderer.get_uniform_buffer().map(sizeof(ParticleRenderer::TransformMatrics), 0);
    memcpy(map, &ptm, sizeof(ParticleRenderer::TransformMatrics));
    this->particle_renderer.get_uniform_buffer().unmap();
}


void ParticlesApp::ParticlesApp::run(void)
{
    std::vector<double> time_stamps;
    double t1s = glfwGetTime();
    while (!glfwGetKey(this->window, GLFW_KEY_ESCAPE) && !glfwWindowShouldClose(this->window))
    {
        double t0 = glfwGetTime();
        glfwPollEvents();
        this->update_frame_contents();
        this->draw_frame();
        double t1 = glfwGetTime();
        time_stamps.push_back(t1 - t0);

        if ((t1 - t1s) >= 1.0)
        {
            double avg = 0.0;
            for (double d : time_stamps)
                avg += d;
            avg /= static_cast<double>(time_stamps.size());
            t1s = glfwGetTime();
            time_stamps.clear();

            std::cout << "\rAVG FPS: " << 1.0 / avg << " AVG MSPF: " << avg * 1000.0 << "                   ";
        }
    }
}

void ParticlesApp::init(void)
{
    this->load_models();
    this->init_glfw();
    this->init_vulkan();
}
