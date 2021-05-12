#define GLFW_INCLUDE_VULKAN
#define VULKAN_ABSTRACTION_DEBUG
#define VULKAN_ABSTRACTION_EXPERIMENTAL

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_absraction.h>
#include <glm/glm.hpp>

#include "particles/particles.h"

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

private:
    struct TransformMatrices
    {
        glm::mat4 MVP;
    };

    /* CONSTANTS */
    constexpr static uint32_t SWAPCHAIN_IMAGES = 3;
    constexpr static VkImageUsageFlags SWAPCHAIN_IMAGE_USAGE = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    constexpr static VkFormat SWAPCHAIN_FORMAT = VK_FORMAT_B8G8R8A8_UNORM;
    constexpr static VkColorSpaceKHR SWAPCHAIN_COLOR_SPACE = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    constexpr static VkPresentModeKHR PRESENTATION_MODE = VK_PRESENT_MODE_MAILBOX_KHR;
    constexpr static VkFormat DEPTH_FORMAT = VK_FORMAT_D24_UNORM_S8_UINT;

    // shader paths
    constexpr static char SHADER_STATIC_SCENE_VERTEX_PATH[] = "../../../assets/shaders/out/static_scene.vert.spv";
    constexpr static char SHADER_STATIC_SCENE_FRAGMENT_PATH[] = "../../../assets/shaders/out/static_scene.frag.spv";
    constexpr static char PARTICLE_VERTEX_PATH[] = "../../../assets/shaders/out/particles.vert.spv";
    constexpr static char PARTICLE_GEOMETRY_PATH[] = "../../../assets/shaders/out/particles.geom.spv";
    constexpr static char PARTICLE_FRAGMENT_PATH[] = "../../../assets/shaders/out/particles.frag.spv";

    // texture path
    constexpr static char TEXTURE_FLOOR[] = "../../../assets/textures/floor_tex.jpg";
    constexpr static char TEXTURE_FOUNTAIN[] = "../../../assets/textures/fountain_tex.jpg";
    constexpr static char TEXTURE_PARTICLE[] = "../../../assets/textures/particle_tex.png";

    // model path
    constexpr static char MODEL_FOUNTAIN[] = "../../../assets/models/fountain.obj";

    // keys
    constexpr static int MOVE_KEY_MAP[6] = { 'W', 'D', 'S', 'A', GLFW_KEY_SPACE, GLFW_KEY_LEFT_SHIFT };

    /* STATIC PRIVATE MEMBERS */
    inline static Config _config;   // config must be global and there will only be one instance

    /* GLFW VARIABLES */
    const GLFWvidmode* vmode;
    GLFWwindow* window;
    int width, height, posx, posy;

    /* VULKAN VARIABLES */
    VkApplicationInfo app_info;
    VkInstance instance;
    VkSurfaceKHR surface;

    VkPhysicalDevice physical_device;
    VkDevice device;
    uint32_t graphics_queue_family_index;
    VkQueue graphics_queue;

    VkSwapchainKHR swapchain;
    std::vector<VkImageView> swapchain_views;
    std::vector<VkFramebuffer> swapchain_fbos;

    vka::AttachmentImage depth_attachment;
    VkRenderPass renderpass_main;

    // pipeline of static scene (floor + fountain)
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;

    VkCommandPool command_pool;
    std::vector<VkCommandBuffer> primary_command_buffers;
    VkCommandBuffer static_scene_command_buffer;

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

    VkDescriptorPool descriptor_pool;
    std::vector<VkDescriptorSetLayout> descriptor_set_layouts;
    std::vector<VkDescriptorSet> descriptor_sets;

    VkSemaphore image_ready, rendering_done;

    ParticleRenderer particle_renderer;
    ParticleEngine particle_engine;

    void load_models(void);
    void load_floor(void);
    void load_fountain(void);

    void init_glfw(void);
    void destry_glfw(void);

    void init_vulkan(void);
    void create_app_info(void);
    void create_instance(void);
    void create_surface(void);

    void create_physical_device(void);
    void create_queues(void);
    void create_device(void);
    void create_swapchain(void);

    void create_depth_attachment(void);
    void create_render_pass(void);

    void create_pipeline(void);

    void create_framebuffers(void);
    void create_command_pool(void);
    void create_command_buffers(void);

    void create_vertex_buffers(void);
    void create_index_buffers(void);
    void create_uniform_buffers(void);
    void create_textures(void);

    void create_desciptor_pool(void);
    void create_desciptor_set_layouts(void);
    void create_descriptor_sets(void);

    void create_semaphores(void);

    void init_particles(void);

    void record_commands(void);
    void record_static_scene(void);
    void record_primary_commands(void);

    void draw_frame(void);
    void destroy_vulkan(void);

    glm::vec3 handle_move_keys(float speed, float yaw, const int keymap[6]);
    void mouse_action(GLFWwindow* window, int width, int height, double& yaw, double& pitch, float sensetivity);
    void move_action(GLFWwindow* window, glm::vec3& pos, const glm::vec3 velocity);
    void update_frame_contents(void);
    
public:
    ParticlesApp(void);
    virtual ~ParticlesApp(void);

    void init(void);
    void run(void);

    inline static Config& config(void) { return _config; };
};