#include "particles.h"

void particle_vertex_t::get_binding_description(std::vector<VkVertexInputBindingDescription>& descr)
{
    descr.resize(1, {});
    descr[0].binding = 0;
    descr[0].stride = sizeof(particle_vertex_t);
    descr[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
}

void particle_vertex_t::get_attribute_description(std::vector<VkVertexInputAttributeDescription>& descr)
{
    descr.resize(3, {});

    descr[0].location = 0;
    descr[0].binding = 0;
    descr[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    descr[0].offset = 0;

    descr[1].location = 1;
    descr[1].binding = 0;
    descr[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    descr[1].offset = 3 * sizeof(float);

    descr[2].location = 2;
    descr[2].binding = 0;
    descr[2].format = VK_FORMAT_R32_SFLOAT;
    descr[2].offset = 7 * sizeof(float);
}