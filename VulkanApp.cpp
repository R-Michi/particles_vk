#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "VulkanApp.h"
#include "random/random.h"

#include <stdexcept>
#include <vector>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <stb/stb_image.h>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

#define VALIDATION_LAYERS 1
#define DISPLAY_SHADOW_MAP 0
#define shader_sizeof(type)     (((sizeof(type) / 16) + 1) * 16)
#define shader_at(type, ptr, i) ((type*)(ptr + shader_sizeof(type) * i))

void __key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
}

ParticlesApp::ParticlesApp(void)
{
    
}

ParticlesApp::~ParticlesApp(void)
{
    
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

// reuse floor as shadow map quad for debug purposes
#if DISPLAY_SHADOW_MAP
    this->floor_vertices = {
        { {+1.0f, -1.0f, 0.0f}, { 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }},
        { {-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} },
        { {-1.0f, +1.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 0.0f} },
        { {+1.0f, +1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 0.0f} }
    };
#endif

    this->floor_indices = {
        1, 0, 3, 3, 2, 1
    };
}

void ParticlesApp::load_fountain(void)
{
    vka::Model323 fountain;
    fountain.load(ParticlesConstants::MODEL_FOUNTAIN);
    fountain.combine(this->fountain_vertices, this->fountain_indices);
}

// ------------------------ LIGHTS ------------------------

void ParticlesApp::init_lights(void)
{
    this->directional_light.direction = glm::normalize(glm::vec3( -2.0f, -1.0f, 1.5f ));
    this->directional_light.intensity = glm::vec3(30.0f);
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

    this->onscreen_renderpass.init(this->physical_device, this->device, this->surface, this->graphics_queue_family_index, this->width, this->height);
    this->directional_shadow_map.init(this->physical_device, this->device, this->graphics_queue_family_index, ParticlesConstants::SHADOW_MAP_RESOLUTION);

    this->create_command_pool();
    this->create_command_buffers();

    this->create_vertex_buffers();
    this->create_index_buffers();
    this->create_uniform_buffers();
    this->create_textures();

    this->init_main_descriptor_manager();
    this->init_dir_shadow_descriptor_manager();

    this->create_pipeline();
    this->create_shadow_pipelines();


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
    filter.reqMinImageCount = ParticlesConstants::SWAPCHAIN_IMAGES;
    filter.reqMaxImageCount = ParticlesConstants::SWAPCHAIN_IMAGES;
    filter.reqSurfaceImageUsageFlags = ParticlesConstants::SWAPCHAIN_IMAGE_USAGE;
    filter.reqSurfaceColorFormats = { ParticlesConstants::SWAPCHAIN_FORMAT };
    filter.reqSurfaceColorSpaces = { ParticlesConstants::SWAPCHAIN_COLOR_SPACE };
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
    features.depthBiasClamp = VK_TRUE;

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


void ParticlesApp::init_main_descriptor_manager(void)
{
    this->main_descr_manager.set_device(this->device);
    this->main_descr_manager.set_number_of_sets(1);

    this->main_descr_manager.add_binding(0, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,           1, VK_SHADER_STAGE_VERTEX_BIT);
    this->main_descr_manager.add_binding(0, 2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,           1, VK_SHADER_STAGE_FRAGMENT_BIT);
    this->main_descr_manager.add_binding(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,   2, VK_SHADER_STAGE_FRAGMENT_BIT);
    this->main_descr_manager.add_binding(0, 3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,           1, VK_SHADER_STAGE_FRAGMENT_BIT);
    this->main_descr_manager.add_binding(0, 4, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,           1, VK_SHADER_STAGE_FRAGMENT_BIT);
    this->main_descr_manager.add_binding(0, 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,   1, VK_SHADER_STAGE_FRAGMENT_BIT);

    VULKAN_ASSERT(this->main_descr_manager.init());

    VkDescriptorBufferInfo buffer_info[4];
    buffer_info[0].buffer = this->tm_buffer.handle();
    buffer_info[0].offset = 0;
    buffer_info[0].range = sizeof(TransformMatrices);

    buffer_info[1].buffer = this->directional_light_buffer.handle();
    buffer_info[1].offset = 0;
    buffer_info[1].range = ParticlesConstants::N_DIRECTIONAL_LIGHTS * sizeof(DirectionalLight);

    buffer_info[2].buffer = this->material_buffer.handle();
    buffer_info[2].offset = 0;
    buffer_info[2].range = ParticlesConstants::N_MATERIALS * sizeof(Material);

    buffer_info[3].buffer = this->fragment_variables_buffer.handle();
    buffer_info[3].offset = 0;
    buffer_info[3].range = sizeof(FragmentVariables);

    VkDescriptorImageInfo image_info[3];
#if DISPLAY_SHADOW_MAP
    image_info[0].sampler = this->dir_shadow_sampler;
    image_info[0].imageView = this->directional_shadow_map.view();
    image_info[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
#else
    image_info[0].sampler = this->floor_texture.sampler();
    image_info[0].imageView = this->floor_texture.view();
    image_info[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
#endif

    image_info[1].sampler = this->fountain_texture.sampler();
    image_info[1].imageView = this->fountain_texture.view();
    image_info[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // drectional shadow map
    image_info[2].sampler = this->directional_shadow_map.sampler;
    image_info[2].imageView = this->directional_shadow_map.depth_attachment.view();
    image_info[2].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    this->main_descr_manager.add_buffer_info(0, 0, 0, 1, buffer_info + 0);
    this->main_descr_manager.add_buffer_info(0, 2, 0, 1, buffer_info + 1);
    this->main_descr_manager.add_buffer_info(0, 3, 0, 1, buffer_info + 2);
    this->main_descr_manager.add_buffer_info(0, 4, 0, 1, buffer_info + 3);
    this->main_descr_manager.add_image_info(0, 1, 0, 2, image_info + 0);
    this->main_descr_manager.add_image_info(0, 5, 0, 1, image_info + 2);

    this->main_descr_manager.update();
}

void ParticlesApp::init_dir_shadow_descriptor_manager(void)
{
    this->dir_shadow_descr_manager.set_device(this->device);
    this->dir_shadow_descr_manager.set_number_of_sets(1);

    this->dir_shadow_descr_manager.add_binding(0, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
    VULKAN_ASSERT(this->dir_shadow_descr_manager.init());

    VkDescriptorBufferInfo descriptor_buffer_info = {};
    descriptor_buffer_info.buffer = this->tm_buffer_dir_shadow.handle();
    descriptor_buffer_info.offset = 0;
    descriptor_buffer_info.range = sizeof(ShadowTransformMatrices);

    this->dir_shadow_descr_manager.add_buffer_info(0, 0, 0, 1, &descriptor_buffer_info);
    this->dir_shadow_descr_manager.update();
}


void ParticlesApp::get_vertex323_descriptions(std::vector<VkVertexInputBindingDescription>& bindings, std::vector<VkVertexInputAttributeDescription>& attributes)
{
    bindings.resize(1, {});
    attributes.resize(3, {});

    bindings[0].binding = 0;
    bindings[0].stride = sizeof(vka::vertex323_t);
    bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    // attribute 0: position
    attributes[0].location = 0;
    attributes[0].binding = 0;
    attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributes[0].offset = 0;

    // attribute 1: texture coordinates
    attributes[1].location = 1;
    attributes[1].binding = 0;
    attributes[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributes[1].offset = sizeof(glm::vec3);

    // attribute 2: normal vector
    attributes[2].location = 2;
    attributes[2].binding = 0;
    attributes[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributes[2].offset = sizeof(glm::vec3) + sizeof(glm::vec2);
}

void ParticlesApp::create_pipeline(void)
{
    vka::Shader vertex(this->device), fragment(this->device);
    vertex.load(ParticlesConstants::SHADER_STATIC_SCENE_VERTEX_PATH, VK_SHADER_STAGE_VERTEX_BIT);
    fragment.load(ParticlesConstants::SHADER_STATIC_SCENE_FRAGMENT_PATH, VK_SHADER_STAGE_FRAGMENT_BIT);

    vka::ShaderProgram shader;
    shader.attach(vertex);
    shader.attach(fragment);
    
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;
    ParticlesApp::get_vertex323_descriptions(bindings, attributes);

    VkPipelineVertexInputStateCreateInfo vertex_input_create_info = {};
    vertex_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_create_info.pNext = nullptr;
    vertex_input_create_info.flags = 0;
    vertex_input_create_info.vertexBindingDescriptionCount = bindings.size();
    vertex_input_create_info.pVertexBindingDescriptions = bindings.data();
    vertex_input_create_info.vertexAttributeDescriptionCount = attributes.size();
    vertex_input_create_info.pVertexAttributeDescriptions = attributes.data();

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
    rasterizer_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
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
    pipeline_layout_create_info.setLayoutCount = this->main_descr_manager.get_layouts().size();
    pipeline_layout_create_info.pSetLayouts = this->main_descr_manager.get_layouts().data();
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
    pipeline_create_info.renderPass = this->onscreen_renderpass.render_pass;
    pipeline_create_info.subpass = 0;
    pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_create_info.basePipelineIndex = -1;

    VULKAN_ASSERT(vkCreateGraphicsPipelines(this->device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &this->pipeline));
}

void ParticlesApp::create_shadow_pipelines(void)
{
    vka::Shader dir_shadow_vert(this->device);
    dir_shadow_vert.load(ParticlesConstants::DIRECTIONAL_SHADOW_VERTEX_PATH, VK_SHADER_STAGE_VERTEX_BIT);
    vka::ShaderProgram dir_shadow_program;
    dir_shadow_program.attach(dir_shadow_vert);

    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;
    ParticlesApp::get_vertex323_descriptions(bindings, attributes);

    VkPipelineVertexInputStateCreateInfo dir_shadow_vert_inp_state = {};
    dir_shadow_vert_inp_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    dir_shadow_vert_inp_state.pNext = nullptr;
    dir_shadow_vert_inp_state.flags = 0;
    dir_shadow_vert_inp_state.vertexBindingDescriptionCount = bindings.size();
    dir_shadow_vert_inp_state.pVertexBindingDescriptions = bindings.data();
    dir_shadow_vert_inp_state.vertexAttributeDescriptionCount = 1;
    dir_shadow_vert_inp_state.pVertexAttributeDescriptions = attributes.data() + 0; // only vertex position description

    VkPipelineInputAssemblyStateCreateInfo dir_shadow_input_assembly_state = {};
    dir_shadow_input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    dir_shadow_input_assembly_state.pNext = nullptr;
    dir_shadow_input_assembly_state.flags = 0;
    dir_shadow_input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    dir_shadow_input_assembly_state.primitiveRestartEnable = VK_FALSE;

    VkViewport dir_shadow_vp = {};
    dir_shadow_vp.x = 0;
    dir_shadow_vp.y = 0;
    dir_shadow_vp.width = ParticlesConstants::SHADOW_MAP_RESOLUTION;
    dir_shadow_vp.height = ParticlesConstants::SHADOW_MAP_RESOLUTION;
    dir_shadow_vp.minDepth = 0.0f;
    dir_shadow_vp.maxDepth = 1.0f;

    VkRect2D dir_shadow_scissor = {};
    dir_shadow_scissor.offset = { 0, 0 };
    dir_shadow_scissor.extent = { ParticlesConstants::SHADOW_MAP_RESOLUTION, ParticlesConstants::SHADOW_MAP_RESOLUTION };

    VkPipelineViewportStateCreateInfo dir_shadow_vp_state = {};
    dir_shadow_vp_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    dir_shadow_vp_state.pNext = nullptr;
    dir_shadow_vp_state.flags = 0;
    dir_shadow_vp_state.viewportCount = 1;
    dir_shadow_vp_state.pViewports = &dir_shadow_vp;
    dir_shadow_vp_state.scissorCount = 1;
    dir_shadow_vp_state.pScissors = &dir_shadow_scissor;

    VkPipelineRasterizationStateCreateInfo dir_shadow_rasterizer_state = {};
    dir_shadow_rasterizer_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    dir_shadow_rasterizer_state.pNext = nullptr;
    dir_shadow_rasterizer_state.flags = 0;
    dir_shadow_rasterizer_state.depthClampEnable = VK_FALSE;
    dir_shadow_rasterizer_state.rasterizerDiscardEnable = VK_FALSE;
    dir_shadow_rasterizer_state.polygonMode = VK_POLYGON_MODE_FILL;
    dir_shadow_rasterizer_state.cullMode = VK_CULL_MODE_BACK_BIT;
    dir_shadow_rasterizer_state.frontFace = VK_FRONT_FACE_CLOCKWISE;
    dir_shadow_rasterizer_state.depthBiasEnable = VK_TRUE;
    dir_shadow_rasterizer_state.depthBiasConstantFactor = 0.0f;
    dir_shadow_rasterizer_state.depthBiasClamp = 0.0f;
    dir_shadow_rasterizer_state.depthBiasSlopeFactor = 0.0f;
    dir_shadow_rasterizer_state.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo dir_shadow_ms_state = {};
    dir_shadow_ms_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    dir_shadow_ms_state.pNext = nullptr;
    dir_shadow_ms_state.flags = 0;
    dir_shadow_ms_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    dir_shadow_ms_state.sampleShadingEnable = VK_FALSE;
    dir_shadow_ms_state.minSampleShading = 1.0f;
    dir_shadow_ms_state.pSampleMask = nullptr;
    dir_shadow_ms_state.alphaToCoverageEnable = VK_FALSE;
    dir_shadow_ms_state.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo dir_shadow_depth_state = {};
    dir_shadow_depth_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    dir_shadow_depth_state.pNext = nullptr;
    dir_shadow_depth_state.flags = 0;
    dir_shadow_depth_state.depthTestEnable = VK_TRUE;
    dir_shadow_depth_state.depthWriteEnable = VK_TRUE;
    dir_shadow_depth_state.depthCompareOp = VK_COMPARE_OP_LESS;
    dir_shadow_depth_state.depthBoundsTestEnable = VK_FALSE;
    dir_shadow_depth_state.stencilTestEnable = VK_FALSE;
    dir_shadow_depth_state.front = {};
    dir_shadow_depth_state.back = {};
    dir_shadow_depth_state.minDepthBounds = 0.0f;
    dir_shadow_depth_state.maxDepthBounds = 1.0f;

    VkDynamicState dynamic_states[3];
    dynamic_states[0] = VK_DYNAMIC_STATE_VIEWPORT;
    dynamic_states[1] = VK_DYNAMIC_STATE_SCISSOR;
    dynamic_states[2] = VK_DYNAMIC_STATE_DEPTH_BIAS;
    // HINT: maybe also depth bias as dynamic state if shadow map resolution changes

    VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {};
    dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_create_info.pNext = nullptr;
    dynamic_state_create_info.flags = 0;
    dynamic_state_create_info.dynamicStateCount = 3;
    dynamic_state_create_info.pDynamicStates = dynamic_states;

    VkPipelineLayoutCreateInfo dir_shadow_layout = {};
    dir_shadow_layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    dir_shadow_layout.pNext = nullptr;
    dir_shadow_layout.flags = 0;
    dir_shadow_layout.setLayoutCount = this->dir_shadow_descr_manager.get_layouts().size();
    dir_shadow_layout.pSetLayouts = this->dir_shadow_descr_manager.get_layouts().data();
    dir_shadow_layout.pushConstantRangeCount = 0;
    dir_shadow_layout.pPushConstantRanges = nullptr;

    VULKAN_ASSERT(vkCreatePipelineLayout(this->device, &dir_shadow_layout, nullptr, &this->pipeline_layout_dir_shadow));

    VkGraphicsPipelineCreateInfo dir_shadow_pipeline = {};
    dir_shadow_pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    dir_shadow_pipeline.pNext = nullptr;
    dir_shadow_pipeline.flags = 0;
    dir_shadow_pipeline.stageCount = dir_shadow_program.count();
    dir_shadow_pipeline.pStages = dir_shadow_program.get_stages();
    dir_shadow_pipeline.pVertexInputState = &dir_shadow_vert_inp_state;
    dir_shadow_pipeline.pInputAssemblyState = &dir_shadow_input_assembly_state;
    dir_shadow_pipeline.pTessellationState = nullptr;
    dir_shadow_pipeline.pViewportState = &dir_shadow_vp_state;
    dir_shadow_pipeline.pRasterizationState = &dir_shadow_rasterizer_state;
    dir_shadow_pipeline.pMultisampleState = &dir_shadow_ms_state;
    dir_shadow_pipeline.pDepthStencilState = &dir_shadow_depth_state;
    dir_shadow_pipeline.pColorBlendState = nullptr;
    dir_shadow_pipeline.pDynamicState = &dynamic_state_create_info;
    dir_shadow_pipeline.layout = this->pipeline_layout_dir_shadow;
    dir_shadow_pipeline.renderPass = this->directional_shadow_map.render_pass;
    dir_shadow_pipeline.subpass = 0;
    dir_shadow_pipeline.basePipelineHandle = VK_NULL_HANDLE;
    dir_shadow_pipeline.basePipelineIndex = -1;

    VULKAN_ASSERT(vkCreateGraphicsPipelines(this->device, VK_NULL_HANDLE, 1, &dir_shadow_pipeline, nullptr, &this->pipeline_dir_shadow));
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
    this->primary_command_buffers.resize(ParticlesConstants::SWAPCHAIN_IMAGES);

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
    VULKAN_ASSERT(vkAllocateCommandBuffers(this->device, &static_scene_cbo_alloc_info, &this->dir_shadow_command_buffer));
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
    // main pipelines's vertex shader's transformation matrices
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

    // buffer for direcional lights (main pipeline)
    this->directional_light_buffer.set_physical_device(this->physical_device);
    this->directional_light_buffer.set_device(this->device);
    this->directional_light_buffer.set_create_flags(0);
    this->directional_light_buffer.set_create_queue_families(&this->graphics_queue_family_index, 1);
    this->directional_light_buffer.set_create_sharing_mode(VK_SHARING_MODE_EXCLUSIVE);
    this->directional_light_buffer.set_create_size(ParticlesConstants::N_DIRECTIONAL_LIGHTS * sizeof(DirectionalLight));
    this->directional_light_buffer.set_create_usage(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    this->directional_light_buffer.set_memory_properties(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
    this->directional_light_buffer.create();

    map = this->directional_light_buffer.map(ParticlesConstants::N_DIRECTIONAL_LIGHTS * sizeof(DirectionalLight), 0);
    memset(map, 0, ParticlesConstants::N_DIRECTIONAL_LIGHTS * sizeof(DirectionalLight));
    this->directional_light_buffer.unmap();

    // buffer for materials (main pipeline)
    this->material_buffer.set_physical_device(this->physical_device);
    this->material_buffer.set_device(this->device);
    this->material_buffer.set_create_flags(0);
    this->material_buffer.set_create_queue_families(&this->graphics_queue_family_index, 1);
    this->material_buffer.set_create_sharing_mode(VK_SHARING_MODE_EXCLUSIVE);
    this->material_buffer.set_create_size(ParticlesConstants::N_MATERIALS * sizeof(Material));
    this->material_buffer.set_create_usage(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    this->material_buffer.set_memory_properties(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
    this->material_buffer.create();

    map = this->material_buffer.map(ParticlesConstants::N_MATERIALS * sizeof(Material), 0);
    memset(map, 0, ParticlesConstants::N_MATERIALS * sizeof(Material));
    this->material_buffer.unmap();

    // buffer for additional values for the fragment shader (main pipeline)
    this->fragment_variables_buffer.set_physical_device(this->physical_device);
    this->fragment_variables_buffer.set_device(this->device);
    this->fragment_variables_buffer.set_create_flags(0);
    this->fragment_variables_buffer.set_create_queue_families(&this->graphics_queue_family_index, 1);
    this->fragment_variables_buffer.set_create_sharing_mode(VK_SHARING_MODE_EXCLUSIVE);
    this->fragment_variables_buffer.set_create_size(sizeof(FragmentVariables));
    this->fragment_variables_buffer.set_create_usage(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    this->fragment_variables_buffer.set_memory_properties(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
    this->fragment_variables_buffer.create();

    map = this->fragment_variables_buffer.map(sizeof(FragmentVariables), 0);
    memset(map, 0, sizeof(FragmentVariables));
    this->fragment_variables_buffer.unmap();

    // transformation matrices for the directional shadow map pipeline
    this->tm_buffer_dir_shadow.set_physical_device(this->physical_device);
    this->tm_buffer_dir_shadow.set_device(this->device);
    this->tm_buffer_dir_shadow.set_create_flags(0);
    this->tm_buffer_dir_shadow.set_create_queue_families(&this->graphics_queue_family_index, 1);
    this->tm_buffer_dir_shadow.set_create_sharing_mode(VK_SHARING_MODE_EXCLUSIVE);
    this->tm_buffer_dir_shadow.set_create_size(sizeof(ShadowTransformMatrices));
    this->tm_buffer_dir_shadow.set_create_usage(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    this->tm_buffer_dir_shadow.set_memory_properties(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
    this->tm_buffer_dir_shadow.create();
}

void ParticlesApp::create_textures(void)
{
    using namespace __internal_random;

    // floor texture
    int w, h, c;
    uint8_t* data = stbi_load(ParticlesConstants::TEXTURE_FLOOR, &w, &h, &c, 4);
    if (data == nullptr)
        std::runtime_error("Failed to load floor texture!");
    size_t px_stride = 4 * sizeof(uint8_t);

    this->floor_texture.set_image_flags(0);
    this->floor_texture.set_image_type(VK_IMAGE_TYPE_2D);
    this->floor_texture.set_image_extent({ static_cast<uint32_t>(w), static_cast<uint32_t>(h), 1 });
    this->floor_texture.set_image_array_layers(1);
    this->floor_texture.set_image_format(VK_FORMAT_R8G8B8A8_UNORM);
    this->floor_texture.set_image_queue_families(this->graphics_queue_family_index);

    this->floor_texture.mipmap_generate(true);
    this->floor_texture.mipmap_filter(VK_FILTER_LINEAR);

    VkImageSubresourceRange srr = {};
    srr.baseMipLevel = 0;
    srr.levelCount = this->floor_texture.mip_levels();
    srr.baseArrayLayer = 0;
    srr.layerCount = 1;

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

    this->floor_texture.set_pyhsical_device(this->physical_device);
    this->floor_texture.set_device(this->device);
    this->floor_texture.set_command_pool(this->command_pool);
    this->floor_texture.set_queue(this->graphics_queue);

    VULKAN_ASSERT(this->floor_texture.create(data, px_stride));
    stbi_image_free(data);

    // fountain texture
    data = stbi_load(ParticlesConstants::TEXTURE_FOUNTAIN, &w, &h, &c, 4);
    if (data == nullptr)
        std::runtime_error("Failed to load fountain texture!");
    px_stride = 4 * sizeof(uint8_t);

    this->fountain_texture.set_image_flags(0);
    this->fountain_texture.set_image_type(VK_IMAGE_TYPE_2D);
    this->fountain_texture.set_image_extent({ static_cast<uint32_t>(w), static_cast<uint32_t>(h), 1 });
    this->fountain_texture.set_image_array_layers(1);
    this->fountain_texture.set_image_format(VK_FORMAT_R8G8B8A8_UNORM);
    this->fountain_texture.set_image_queue_families(this->graphics_queue_family_index);

    this->fountain_texture.mipmap_generate(true);
    this->fountain_texture.mipmap_filter(VK_FILTER_LINEAR);

    srr.levelCount = this->fountain_texture.mip_levels();

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

    this->fountain_texture.set_pyhsical_device(this->physical_device);
    this->fountain_texture.set_device(this->device);
    this->fountain_texture.set_command_pool(this->command_pool);
    this->fountain_texture.set_queue(this->graphics_queue);

    VULKAN_ASSERT(this->fountain_texture.create(data, px_stride));
    stbi_image_free(data);
}


void ParticlesApp::create_semaphores(void)
{
    VkSemaphoreCreateInfo sem_create_info = {};
    sem_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    sem_create_info.pNext = nullptr;
    sem_create_info.flags = 0;
    
    VULKAN_ASSERT(vkCreateSemaphore(this->device, &sem_create_info, nullptr, &this->image_ready));
    VULKAN_ASSERT(vkCreateSemaphore(this->device, &sem_create_info, nullptr, &this->rendering_done));

    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.pNext = nullptr;
    fence_info.flags = 0;

    VULKAN_ASSERT(vkCreateFence(this->device, &fence_info, nullptr, &this->render_fence));
}


void ParticlesApp::init_particles(void)
{
    constexpr static size_t n_particles = 1000;

    vka::Shader particle_vert(this->device), particle_geom(this->device), particle_frag(this->device);
    particle_vert.load(ParticlesConstants::PARTICLE_VERTEX_PATH, VK_SHADER_STAGE_VERTEX_BIT);
    particle_geom.load(ParticlesConstants::PARTICLE_GEOMETRY_PATH, VK_SHADER_STAGE_GEOMETRY_BIT);
    particle_frag.load(ParticlesConstants::PARTICLE_FRAGMENT_PATH, VK_SHADER_STAGE_FRAGMENT_BIT);

    vka::ShaderProgram program;
    program.attach(particle_vert);
    program.attach(particle_geom);
    program.attach(particle_frag);

    ParticleRenderer::Config particle_config = {};
    particle_config.physical_device = this->physical_device;
    particle_config.device = this->device;
    particle_config.render_pass = this->onscreen_renderpass.render_pass;
    particle_config.subpass = 0;
    particle_config.family_index = this->graphics_queue_family_index;
    particle_config.graphics_queue = this->graphics_queue;
    particle_config.p_program = &program;
    particle_config.texture_path = ParticlesConstants::TEXTURE_PARTICLE;
    particle_config.scr_width = this->width;
    particle_config.scr_height = this->height;
    particle_config.max_particles = n_particles;

    this->particle_renderer.init(particle_config);
    this->particle_renderer.record(particle_config);

#if 0
    Particle particle1({ 4.0f,4.0f,4.0f }, { 0.0f,1.0f,0.0f,0.5f }, 0.5f, { 0.0f,0.0f,0.0f }, std::chrono::milliseconds(0));
    Particle particle2({ 5.0f,4.0f,4.0f }, { 0.0f,0.0f,1.0f,0.5f }, 0.5f, { 0.0f,0.0f,0.0f }, std::chrono::milliseconds(0));

    particle_vertex_t* map = (particle_vertex_t*)this->particle_renderer.get_vertex_buffer().map(2 * sizeof(particle_vertex_t), 0);
    map[0] = particle1.vertex();
    map[1] = particle2.vertex();
    this->particle_renderer.get_vertex_buffer().unmap();
#else
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
    engine_config.min_ttl = std::chrono::milliseconds(15000);
    engine_config.max_ttl = std::chrono::milliseconds(16000);

    this->particle_engine.init(engine_config);
    this->particle_engine.start(std::chrono::milliseconds(3000));
#endif
}

void ParticlesApp::record_commands(void)
{
    /* recording of particles's command buffer is in method "init_particles()" */
    this->record_dir_shadow_map();
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

    for (size_t i = 0; i < this->onscreen_renderpass.views.size(); i++)
    {
        VULKAN_ASSERT(vkBeginCommandBuffer(this->primary_command_buffers[i], &command_begin_info));

        // draw directional shadow map
        VkRect2D dir_shadow_render_area = {};
        dir_shadow_render_area.offset = { 0, 0 };
        dir_shadow_render_area.extent = { ParticlesConstants::SHADOW_MAP_RESOLUTION, ParticlesConstants::SHADOW_MAP_RESOLUTION };

        VkClearValue dir_shadow_clear_value = {};
        dir_shadow_clear_value.depthStencil.depth = 1.0f;

        VkRenderPassBeginInfo dir_shadow_render_pass_begin_info = {};
        dir_shadow_render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        dir_shadow_render_pass_begin_info.pNext = nullptr;
        dir_shadow_render_pass_begin_info.renderPass = this->directional_shadow_map.render_pass;
        dir_shadow_render_pass_begin_info.framebuffer = this->directional_shadow_map.fbo;
        dir_shadow_render_pass_begin_info.renderArea = dir_shadow_render_area;
        dir_shadow_render_pass_begin_info.clearValueCount = 1;
        dir_shadow_render_pass_begin_info.pClearValues = &dir_shadow_clear_value;

        vkCmdBeginRenderPass(this->primary_command_buffers[i], &dir_shadow_render_pass_begin_info, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
        vkCmdExecuteCommands(this->primary_command_buffers[i], 1, &this->dir_shadow_command_buffer);
        vkCmdEndRenderPass(this->primary_command_buffers[i]);

        // draw main scene
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
        render_pass_begin_info.renderPass = this->onscreen_renderpass.render_pass;
        render_pass_begin_info.framebuffer = this->onscreen_renderpass.fbos[i];
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
    inheritance_info.renderPass = this->onscreen_renderpass.render_pass;
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
    vkCmdBindDescriptorSets(this->static_scene_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline_layout, 0, this->main_descr_manager.get_sets().size(), this->main_descr_manager.get_sets().data(), 0, nullptr);

    vkCmdDrawIndexed(this->static_scene_command_buffer, this->floor_indices.size(), 1, 0, 0, 0);

#if !DISPLAY_SHADOW_MAP
    // draw fountain
    vkCmdBindVertexBuffers(this->static_scene_command_buffer, 0, 1, buffers + 1, offsets + 1);
    vkCmdBindIndexBuffer(this->static_scene_command_buffer, this->fountain_index_buffer.handle(), 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(this->static_scene_command_buffer, this->fountain_indices.size(), 1, 0, 0, 1);
#endif

    VULKAN_ASSERT(vkEndCommandBuffer(this->static_scene_command_buffer));
}

void ParticlesApp::record_dir_shadow_map(void)
{
    VkCommandBufferInheritanceInfo inheritance_info = {};
    inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    inheritance_info.pNext = nullptr;
    inheritance_info.renderPass = this->directional_shadow_map.render_pass;
    inheritance_info.subpass = 0;
    inheritance_info.framebuffer = this->directional_shadow_map.fbo;
    inheritance_info.occlusionQueryEnable = VK_FALSE;
    inheritance_info.queryFlags = 0;
    inheritance_info.pipelineStatistics = 0;

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pNext = nullptr;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    begin_info.pInheritanceInfo = &inheritance_info;

    VULKAN_ASSERT(vkBeginCommandBuffer(this->dir_shadow_command_buffer, &begin_info));

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = ParticlesConstants::SHADOW_MAP_RESOLUTION;
    viewport.height = ParticlesConstants::SHADOW_MAP_RESOLUTION;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = { ParticlesConstants::SHADOW_MAP_RESOLUTION, ParticlesConstants::SHADOW_MAP_RESOLUTION };

    float min_depth_bias = 200.0f;
    float max_depth_bias = 200.0f;
    float depth_slope_factor = 0.0f;

    vkCmdBindPipeline(this->dir_shadow_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline_dir_shadow);
    vkCmdSetViewport(this->dir_shadow_command_buffer, 0, 1, &viewport);
    vkCmdSetScissor(this->dir_shadow_command_buffer, 0, 1, &scissor);
    vkCmdSetDepthBias(this->dir_shadow_command_buffer, min_depth_bias, max_depth_bias, depth_slope_factor);

    // draw floor to shadow map
    VkBuffer buffers[2] = { this->floor_vertex_buffer.handle(), this->fountain_vertex_buffer.handle() };
    VkDeviceSize offsets[2] = { 0, 0 };
    vkCmdBindVertexBuffers(this->dir_shadow_command_buffer, 0, 1, buffers + 0, offsets + 0);
    vkCmdBindIndexBuffer(this->dir_shadow_command_buffer, this->floor_index_buffer.handle(), 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(this->dir_shadow_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline_layout_dir_shadow, 0, this->dir_shadow_descr_manager.get_sets().size(), this->dir_shadow_descr_manager.get_sets().data(), 0, nullptr);

#if !DISPLAY_SHADOW_MAP
    vkCmdDrawIndexed(this->dir_shadow_command_buffer, this->floor_indices.size(), 1, 0, 0, 0);
#endif

    // draw fountain to shadow map
    vkCmdBindVertexBuffers(this->dir_shadow_command_buffer, 0, 1, buffers + 1, offsets + 1);
    vkCmdBindIndexBuffer(this->dir_shadow_command_buffer, this->fountain_index_buffer.handle(), 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(this->dir_shadow_command_buffer, this->fountain_indices.size(), 1, 0, 0, 1);

    VULKAN_ASSERT(vkEndCommandBuffer(this->dir_shadow_command_buffer));
}


void ParticlesApp::draw_frame(void)
{
    uint32_t img_index;
    VULKAN_ASSERT(vkAcquireNextImageKHR(this->device, this->onscreen_renderpass.swapchain, ~(0UI64), this->image_ready, VK_NULL_HANDLE, &img_index));

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
    if(this->render_time > 0.006)   // 0.006s -> 6ms
        vkQueueWaitIdle(this->graphics_queue);

    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = nullptr;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &this->rendering_done;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &this->onscreen_renderpass.swapchain;
    present_info.pImageIndices = &img_index;
    present_info.pResults = nullptr;

    VULKAN_ASSERT(vkQueuePresentKHR(this->graphics_queue, &present_info));
}

void ParticlesApp::destroy_vulkan(void)
{
    vkDeviceWaitIdle(this->device);

    this->particle_engine.stop();
    this->particle_renderer.clear();

    vkDestroyFence(this->device, this->render_fence, nullptr);
    vkDestroySemaphore(this->device, this->image_ready, nullptr);
    vkDestroySemaphore(this->device, this->rendering_done, nullptr);

    vkDestroyPipeline(this->device, this->pipeline, nullptr);
    vkDestroyPipelineLayout(this->device, this->pipeline_layout, nullptr);
    vkDestroyPipeline(this->device, this->pipeline_dir_shadow, nullptr);
    vkDestroyPipelineLayout(this->device, this->pipeline_layout_dir_shadow, nullptr);

    this->main_descr_manager.clear();
    this->dir_shadow_descr_manager.clear();

    this->floor_texture.clear();
    this->floor_vertex_buffer.clear();
    this->floor_index_buffer.clear();
    this->fountain_vertex_buffer.clear();
    this->fountain_index_buffer.clear();
    this->fountain_texture.clear();
    this->tm_buffer.clear();
    this->tm_buffer_dir_shadow.clear();
    this->directional_light_buffer.clear();
    this->material_buffer.clear();
    this->fragment_variables_buffer.clear();

    vkFreeCommandBuffers(this->device, this->command_pool, this->primary_command_buffers.size(), this->primary_command_buffers.data());
    vkFreeCommandBuffers(this->device, this->command_pool, 1, &this->static_scene_command_buffer);
    vkFreeCommandBuffers(this->device, this->command_pool, 1, &this->dir_shadow_command_buffer);
    vkDestroyCommandPool(this->device, this->command_pool, nullptr);

    this->directional_shadow_map.clear(this->device);
    this->onscreen_renderpass.clear(this->device);

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
    _config.cam.velocity = this->handle_move_keys(_config.movement_speed, _config.cam.yaw, ParticlesConstants::MOVE_KEY_MAP);
    this->mouse_action(this->window, this->width, this->height, _config.cam.yaw, _config.cam.pitch, _config.sesitivity);
    this->move_action(this->window, _config.cam.pos, _config.cam.velocity);

    // shadow map MVP matrix
    glm::mat4 dir_shadow_view = glm::lookAt(-this->directional_light.direction * 5.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f });
    glm::mat4 dir_shadow_projection = glm::ortho(-2.5f, 2.5f, -2.5f, 2.5f, 0.0f, 15.0f);

    ShadowTransformMatrices dir_shadow_tm;
    dir_shadow_tm.MVP = dir_shadow_projection * dir_shadow_view;
    void* map = this->tm_buffer_dir_shadow.map(sizeof(ShadowTransformMatrices), 0);
    memcpy(map, &dir_shadow_tm, sizeof(ShadowTransformMatrices));
    this->tm_buffer_dir_shadow.unmap();

    // main shader MVP matrix
    glm::mat4 model(1.0f);
    glm::mat4 view = glm::lookAt(glm::dvec3(_config.cam.pos.x, _config.cam.pos.y, _config.cam.pos.z),
                                 glm::dvec3(_config.cam.pos.x + sin(_config.cam.yaw) * cos(_config.cam.pitch),
                                            _config.cam.pos.y + sin(_config.cam.pitch),
                                            _config.cam.pos.z + cos(_config.cam.yaw) * cos(_config.cam.pitch)),
                                 glm::dvec3(0.0, -1.0, 0.0));
    glm::mat4 projection = glm::perspective(glm::radians(100.0f), static_cast<float>(this->width) / static_cast<float>(this->height), 0.001f, 100.0f);

    TransformMatrices tm;
#if DISPLAY_SHADOW_MAP
    tm.MVP = glm::mat4(1.0f);
    tm.light_MVP = glm::mat4(1.0f);
#else
    tm.MVP = projection * view * model;
    tm.light_MVP = dir_shadow_projection * dir_shadow_view;
#endif
    map = this->tm_buffer.map(sizeof(TransformMatrices), 0);
    memcpy(map, &tm, sizeof(TransformMatrices));
    this->tm_buffer.unmap();

    // particle shader MVP matrix
    ParticleRenderer::TransformMatrics ptm;
    ptm.view = view;
    ptm.projection = projection;

    map = this->particle_renderer.get_uniform_buffer().map(sizeof(ParticleRenderer::TransformMatrics), 0);
    memcpy(map, &ptm, sizeof(ParticleRenderer::TransformMatrics));
    this->particle_renderer.get_uniform_buffer().unmap();

    // main shader fragment variables
    FragmentVariables* fv = (FragmentVariables*)this->fragment_variables_buffer.map(sizeof(FragmentVariables), 0);
    fv->cam_pos = _config.cam.pos;
    fv->shadow_penumbra_size = 0.003f;
    fv->shadow_samples = 16;
    this->fragment_variables_buffer.unmap();
}


void ParticlesApp::update_lights(void)
{
    int8_t* map = (int8_t*)this->directional_light_buffer.map(ParticlesConstants::N_DIRECTIONAL_LIGHTS * sizeof(DirectionalLight), 0);

    DirectionalLight* light = shader_at(DirectionalLight, map, 0);
    *light = this->directional_light;
    this->directional_light_buffer.unmap();
}

void ParticlesApp::update_materials(void)
{
    // get byte pointer of mapped memory
    int8_t* map = (int8_t*)this->material_buffer.map(ParticlesConstants::N_MATERIALS * sizeof(Material), 0);

    // floor material
    Material* material = shader_at(Material, map, 0); // first mateial
    material->albedo = glm::vec3(0.5f);
    material->roughness = 0.9f;
    material->metallic = 0.0f;
    material->alpha = 1.0f;

    // fountain material
    material = shader_at(Material, map, 1); // second material
    material->albedo = glm::vec3(0.5f);
    material->roughness = 0.2;
    material->metallic = 0.0f;
    material->alpha = 1.0f;

    this->material_buffer.unmap();
}


float ParticlesApp::get_depth_bias(float bias, uint32_t depth_bits)
{
    return bias * powf(2, depth_bits);
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
            this->render_time = avg;
            t1s = glfwGetTime();
            time_stamps.clear();

            std::cout << "\rAVG FPS: " << 1.0 / avg << " AVG MSPF: " << avg * 1000.0 << "                   ";
        }
    }
}

void ParticlesApp::init(void)
{
    this->load_models();
    this->init_lights();
    this->init_glfw();
    this->init_vulkan();

    this->update_lights();
    this->update_materials();
}

void ParticlesApp::shutdown(void)
{
    this->destroy_vulkan();
    this->destry_glfw();
}