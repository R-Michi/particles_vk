#pragma once

#define GLFW_INCLUDE_VULKAN
#define VULKAN_ABSTRACTION_DEBUG
#define VULKAN_ABSTRACTION_EXPERIMENTAL

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_absraction.h>
#include <glm/glm.hpp>
#include <thread>
#include <atomic>

#include "particles/particles.h"

namespace ParticlesConstants
{
    /* CONSTANTS */
    constexpr static uint32_t SWAPCHAIN_IMAGES = 3;
    constexpr static VkImageUsageFlags SWAPCHAIN_IMAGE_USAGE = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    constexpr static VkFormat SWAPCHAIN_FORMAT = VK_FORMAT_B8G8R8A8_UNORM;
    constexpr static VkColorSpaceKHR SWAPCHAIN_COLOR_SPACE = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    constexpr static VkPresentModeKHR PRESENTATION_MODE = VK_PRESENT_MODE_MAILBOX_KHR;
    constexpr static VkFormat DEPTH_FORMAT = VK_FORMAT_D24_UNORM_S8_UINT;
    constexpr static VkFormat SHADOW_DEPTH_FORMAT = VK_FORMAT_D16_UNORM;
    constexpr static size_t N_DIRECTIONAL_LIGHTS = 1;
    constexpr static size_t N_MATERIALS = 2;
    constexpr static uint32_t SHADOW_MAP_RESOLUTION = 2048;
    constexpr static uint32_t SHADOW_MAP_KERNEL_RADIUS = 2;
    constexpr static uint32_t SHADOW_MAP_SAMPLES = 16;
    constexpr static uint32_t SHADOW_MAP_SAMPLES_DIV_2 = SHADOW_MAP_SAMPLES / 2;
    constexpr static float SHADOW_PENUMBRA_SIZE = 2.0f;

    // shader paths
    constexpr static char SHADER_STATIC_SCENE_VERTEX_PATH[] = "../../../assets/shaders/out/static_scene.vert.spv";
    constexpr static char SHADER_STATIC_SCENE_FRAGMENT_PATH[] = "../../../assets/shaders/out/static_scene.frag.spv";
    constexpr static char PARTICLE_VERTEX_PATH[] = "../../../assets/shaders/out/particles.vert.spv";
    constexpr static char PARTICLE_GEOMETRY_PATH[] = "../../../assets/shaders/out/particles.geom.spv";
    constexpr static char PARTICLE_FRAGMENT_PATH[] = "../../../assets/shaders/out/particles.frag.spv";
    constexpr static char DIRECTIONAL_SHADOW_VERTEX_PATH[] = "../../../assets/shaders/out/dir_shadow.vert.spv";

    // texture path
    constexpr static char TEXTURE_FLOOR[] = "../../../assets/textures/floor_tex.jpg";
    constexpr static char TEXTURE_FOUNTAIN[] = "../../../assets/textures/fountain_tex.jpg";
    constexpr static char TEXTURE_PARTICLE[] = "../../../assets/textures/particle_tex.png";

    // model path
    constexpr static char MODEL_FOUNTAIN[] = "../../../assets/models/fountain.obj";

    // keys
    constexpr static int MOVE_KEY_MAP[6] = { 'W', 'D', 'S', 'A', GLFW_KEY_SPACE, GLFW_KEY_LEFT_SHIFT };
};

struct OnscreenRenderPass
{
    VkSwapchainKHR swapchain{ VK_NULL_HANDLE };
    vka::AttachmentImage depth_attachment;
    VkRenderPass render_pass;
    std::vector<VkImageView> views; // color attachments
    std::vector<VkFramebuffer> fbos;

    void init(VkPhysicalDevice physical_device, VkDevice device, VkSurfaceKHR surface, uint32_t queue_fam_index, uint32_t width, uint32_t height);
    void reshape(VkPhysicalDevice physical_device, VkDevice device, VkSurfaceKHR surface, uint32_t queue_fam_index, uint32_t width, uint32_t height);
    void clear(VkDevice device);

private:
    void init_swapchain(VkDevice device, VkSurfaceKHR surface, uint32_t queue_fam_index, uint32_t width, uint32_t height);
    void init_depth_attachment(VkPhysicalDevice physical_device, VkDevice device, uint32_t queue_fam_index, uint32_t width, uint32_t height);
    void init_render_pass(VkDevice device);
    void init_fbos(VkDevice device, uint32_t width, uint32_t height);

    void destroy_fbos(VkDevice device);
    void destroy_views(VkDevice device);
};

struct ShadowMap
{
    vka::AttachmentImage depth_attachment;
    VkSampler sampler;
    VkRenderPass render_pass;
    VkFramebuffer fbo;

    void init(VkPhysicalDevice physical_device, VkDevice device, uint32_t queue_fam_index, uint32_t resolution);
    void reshape(VkPhysicalDevice physical_device, VkDevice device, uint32_t queue_fam_index, uint32_t resolution);
    void clear(VkDevice device);

private:
    void init_depth_attachment(VkPhysicalDevice physical_device, VkDevice device, uint32_t queue_fam_index, uint32_t resolution);
    void init_sampler(VkDevice device);
    void init_render_pass(VkDevice device);
    void init_fbo(VkDevice device, uint32_t resolution);
};

class DescriptorManager
{
private:
    // handles
    VkDescriptorPool pool;
    std::vector<VkDescriptorSetLayout> set_layouts;
    std::vector<VkDescriptorSet> sets;

    // descriptor structs
    std::vector<VkDescriptorPoolSize> pool_sizes;
    std::vector< std::vector<VkDescriptorSetLayoutBinding> > set_layout_bindings;
    std::vector<VkWriteDescriptorSet> descriptor_writes;

    // requiered
    VkDevice device;

    VkResult init_descriptor_layouts(void);
    VkResult init_descriptor_pool(void);
    VkResult init_descriptor_sets(void);

    void add_to_pool_sizes(VkDescriptorType type, uint32_t count);
    void init_descriptor_write(VkWriteDescriptorSet& write, uint32_t set, uint32_t binding, uint32_t begin_element, uint32_t n_elements);

public:
    DescriptorManager(void);
    DescriptorManager(VkDevice device, uint32_t n_sets);
    virtual ~DescriptorManager(void);

    void set_device(VkDevice device);
    void set_number_of_sets(uint32_t n_sets);

    // methods for initialization
    // these methods MUST NOT be called after initialization
    void add_binding(uint32_t set, uint32_t binding, VkDescriptorType type, uint32_t count, VkShaderStageFlags stage);

    // methods for update
    void add_buffer_info(uint32_t set, uint32_t binding, uint32_t begin_element, uint32_t n_elements, const VkDescriptorBufferInfo* buffer_infos);
    void add_image_info(uint32_t set, uint32_t binding, uint32_t begin_element, uint32_t n_elements, const VkDescriptorImageInfo* image_infos);
    void add_texel_buffer_view(uint32_t set, uint32_t binding, uint32_t begin_element, uint32_t n_elements, const VkBufferView* buffer_views);

    VkResult init(void);
    void update(void);
    void clear(void);

    VkDescriptorPool get_pool(void) const noexcept { return this->pool; }
    const std::vector<VkDescriptorSetLayout>& get_layouts(void) const noexcept { return this->set_layouts; }
    const std::vector<VkDescriptorSet>& get_sets(void) const noexcept  { return this->sets; }
};


class ParticlesApp
{
public:
    struct Camera
    {
        glm::vec3 velocity;
        glm::vec3 pos;
        double yaw, pitch;
    };

    struct Config
    {
        Camera cam;
        float movement_speed;
        float sesitivity;
    };

    struct DirectionalLight
    {
        alignas(16) glm::vec3 direction;
        alignas(16) glm::vec3 intensity;
    };

    struct Material
    {
        glm::vec3 albedo;
        float roughness;
        float metallic;
        float alpha;
    };

    struct FragmentVariables
    {
        alignas(16) glm::vec3 cam_pos;
        alignas(4)  float shadow_penumbra_size;
        alignas(4)  int shadow_samples;
    };

    struct ShadowTransformMatrices
    {
        glm::mat4 MVP;
    };

    struct TransformMatrices
    {
        glm::mat4 MVP;
        glm::mat4 light_MVP;
    };

private:

    /* STATIC PRIVATE MEMBERS */
    inline static Config _config;   // config must be global and there will only be one instance

    /* GLFW VARIABLES */
    const GLFWvidmode* vmode;
    GLFWwindow* window;
    int width, height, posx, posy;
    double render_time = 0.0;

    /* VULKAN VARIABLES */
    VkApplicationInfo app_info;
    VkInstance instance;
    VkSurfaceKHR surface;

    VkPhysicalDevice physical_device;
    VkDevice device;
    uint32_t graphics_queue_family_index;
    VkQueue graphics_queue;

    OnscreenRenderPass onscreen_renderpass;
    ShadowMap directional_shadow_map;

    DescriptorManager main_descr_manager;
    DescriptorManager dir_shadow_descr_manager;

    // pipeline of static scene (floor + fountain)
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;
    
    VkPipelineLayout pipeline_layout_dir_shadow;
    VkPipeline pipeline_dir_shadow;

    VkCommandPool command_pool;
    std::vector<VkCommandBuffer> primary_command_buffers;
    VkCommandBuffer static_scene_command_buffer;
    VkCommandBuffer dir_shadow_command_buffer;

    // floor
    std::vector<vka::vertex323_t> floor_vertices;
    std::vector<uint32_t> floor_indices;
    vka::Buffer floor_vertex_buffer, floor_index_buffer;
    vka::Texture floor_texture;

    // fountain
    std::vector<vka::vertex323_t> fountain_vertices;
    std::vector<uint32_t> fountain_indices;
    vka::Buffer fountain_vertex_buffer, fountain_index_buffer;
    vka::Texture fountain_texture;

    // uniform buffers
    vka::Buffer tm_buffer;  // transform matrices buffer
    vka::Buffer directional_light_buffer;
    vka::Buffer material_buffer;
    vka::Buffer fragment_variables_buffer;
    vka::Buffer tm_buffer_dir_shadow;

    VkSemaphore image_ready, rendering_done;
    VkFence render_fence;

    DirectionalLight directional_light;
    particles::ParticleRenderer particle_renderer;
    std::thread application_thread;
    std::atomic_bool renderer_shutdown;

    void load_models(void);
    void load_floor(void);
    void load_fountain(void);

    void init_lights(void);

    void init_glfw(void);
    void destry_glfw(void);

    void init_vulkan(void);
    void reshape(void);
    void create_app_info(void);
    void create_instance(void);
    void create_surface(void);

    void create_physical_device(void);
    void create_queues(void);
    void create_device(void);

    void init_main_descriptor_manager(void);
    void init_dir_shadow_descriptor_manager(void);

    static void get_vertex323_descriptions(std::vector<VkVertexInputBindingDescription>& bindings, std::vector<VkVertexInputAttributeDescription>& attributes);
    void create_pipeline(void);
    void create_shadow_pipelines(void);

    void create_command_pool(void);
    void create_command_buffers(void);

    void create_vertex_buffers(void);
    void create_index_buffers(void);
    void create_uniform_buffers(void);
    void create_textures(void);

    void create_semaphores(void);

    void init_particles(void);

    void record_commands(void);
    void record_static_scene(void);
    void record_dir_shadow_map(void);
    void record_primary_commands(void);

    void draw_frame(void);
    void destroy_vulkan(void);

    glm::vec3 handle_move_keys(float speed, float yaw, const int keymap[6]);
    void mouse_action(GLFWwindow* window, int width, int height, double& yaw, double& pitch, float sensetivity);
    void move_action(GLFWwindow* window, glm::vec3& pos, const glm::vec3 velocity);
    void update_frame_contents(void);

    void update_lights(void);
    void update_materials(void);

    static float get_depth_bias(float bias, uint32_t depth_bits);

    // this thread separates the renderer thread (main thread) from updating physics, animations, etc.
    // but has full access to the contents of the renderer thread, like a new 'int main()' but with an
    // rendering engine in the background
    void start_application_thread(void);
    void stop_application_thread(void);
    static void application_main(ParticlesApp*);

public:
    ParticlesApp(void);
    virtual ~ParticlesApp(void);

    void init(void);
    void run(void);
    void shutdown(void);

    inline static Config& config(void) { return _config; };
};
