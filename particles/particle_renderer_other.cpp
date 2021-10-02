#include "particle_renderer.h"
#include <stdexcept>
#include <thread>

using namespace particles;

void ParticleRenderer::set_view(const glm::mat4& v) noexcept
{
    if (this->_initialized)
    {
        this->transformation_matrices->view = v;
    }
}

void ParticleRenderer::set_projection(const glm::mat4& p) noexcept
{
    if (this->_initialized)
    {
        this->transformation_matrices->projection = p;
    }
}

void ParticleRenderer::set_view_projection(const glm::mat4& v, const glm::mat4& p) noexcept
{
    if (this->_initialized)
    {
        this->transformation_matrices->view = v;
        this->transformation_matrices->projection = p;
    }
}

VkResult ParticleRenderer::record(const ParticleRendererRecordInfo& info)
{
    if (!this->_initialized)
        throw std::runtime_error("ParticleRenderer must be initialized before recording commands.");

    VkCommandBufferInheritanceInfo inheritance_info = {};
    inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    inheritance_info.pNext = nullptr;
    inheritance_info.renderPass = info.render_pass;
    inheritance_info.subpass = info.sub_pass;
    inheritance_info.framebuffer = info.framebuffer;
    inheritance_info.occlusionQueryEnable = VK_FALSE;
    inheritance_info.queryFlags = 0;
    inheritance_info.pipelineStatistics = 0;

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pNext = nullptr;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    begin_info.pInheritanceInfo = &inheritance_info;

    VkResult result = vkBeginCommandBuffer(this->command_buffer, &begin_info);
    if (result != VK_SUCCESS) return result;

    vkCmdBindPipeline(this->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline);
    vkCmdSetViewport(this->command_buffer, 0, 1, &info.viewport);
    vkCmdSetScissor(this->command_buffer, 0, 1, &info.scissor);
    vkCmdBindDescriptorSets(this->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline_layout, 0, 1, &this->descriptor_set, 0, nullptr);

    VkDeviceSize offset = 0;
    VkBuffer vertex_buffer = this->particle_buffer.handle();
    vkCmdBindVertexBuffers(this->command_buffer, 0, 1, &vertex_buffer, &offset);
    
    vkCmdDrawIndirect(this->command_buffer, this->indirect_buffer.handle(), 0, 1, sizeof(VkDrawIndirectCommand));

    return vkEndCommandBuffer(this->command_buffer);
}