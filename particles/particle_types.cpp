#include "particle_types.h"
#include <stdexcept>

using namespace particles;

void particle_t::get_binding_description(std::vector<VkVertexInputBindingDescription>& bindings)
{
    bindings.resize(1, {});
    bindings[0].binding = 0;
    bindings[0].stride = sizeof(particle_t);
    bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
}

void particle_t::get_attribute_description(std::vector<VkVertexInputAttributeDescription>& attributes)
{
    attributes.resize(3, {});
    attributes[0].location = 0;
    attributes[0].binding = 0;
    attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributes[0].offset = 0;

    attributes[1].location = 1;
    attributes[1].binding = 0;
    attributes[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributes[1].offset = 3 * sizeof(float);

    attributes[2].location = 2;
    attributes[2].binding = 0;
    attributes[2].format = VK_FORMAT_R32_SFLOAT;
    attributes[2].offset = 7 * sizeof(float);
}