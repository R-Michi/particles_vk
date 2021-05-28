#include "../VulkanApp.h"

#include <iostream>

DescriptorManager::DescriptorManager(void)
{
    this->pool = VK_NULL_HANDLE;
    this->device = VK_NULL_HANDLE;
}

DescriptorManager::DescriptorManager(VkDevice device, uint32_t n_sets)
{
    this->pool = VK_NULL_HANDLE;
    this->device = device;
    this->set_number_of_sets(n_sets);
}

DescriptorManager::~DescriptorManager(void)
{
    this->clear();
}


void DescriptorManager::add_to_pool_sizes(VkDescriptorType type, uint32_t count)
{
    // find descriptor type
    for (VkDescriptorPoolSize& pool_size : this->pool_sizes)
    {
        if (pool_size.type == type)
        {
            // sum up the count of this descriptor type
            pool_size.descriptorCount += count;
            return;
        }
    }

    // if pool size with this descriptor type does not exist, create it
    VkDescriptorPoolSize pool_size = {};
    pool_size.type = type;
    pool_size.descriptorCount = count;
    this->pool_sizes.push_back(pool_size);
}


void DescriptorManager::set_device(VkDevice device)
{
    this->device = device;
}

void DescriptorManager::set_number_of_sets(uint32_t n_sets)
{
    if (this->pool != VK_NULL_HANDLE)
        throw std::runtime_error("DescriptorManager::set_number_of_sets MUST NOT be called after DescriptorManager::init!");

    this->set_layout_bindings.resize(n_sets);
}

void DescriptorManager::add_binding(uint32_t set, uint32_t binding, VkDescriptorType type, uint32_t count, VkShaderStageFlags stage)
{
    if (this->pool != VK_NULL_HANDLE)
        throw std::runtime_error("DescriptorManager::add_binding MUST NOT be called after DescriptorManager::init!");
    if (set >= this->set_layout_bindings.size())
        throw std::out_of_range("Parameter set of DescriptorManager::add_buffer_binding is bigger than the amount of sets in the DescriptorManager!");

    // add binding to descriptor set
    VkDescriptorSetLayoutBinding set_binding = {};
    set_binding.binding = binding;
    set_binding.descriptorType = type;
    set_binding.descriptorCount = count;
    set_binding.stageFlags = stage;
    set_binding.pImmutableSamplers = nullptr;
    this->set_layout_bindings.at(set).push_back(set_binding);

    // add the descriptors of the binding to the pool sizes
    this->add_to_pool_sizes(type, count);
}


void DescriptorManager::init_descriptor_write(VkWriteDescriptorSet& write, uint32_t set, uint32_t binding, uint32_t begin_element, uint32_t n_elements)
{
    if (set >= this->set_layout_bindings.size())
        throw std::out_of_range("Parameter set of DescriptorManager::init_descriptor_write is bigger than the amount of sets in the DescriptorManager!");

    // binding number does not correspond to the index of the vector
    // search the vector index for this binding
    int64_t index = -1;
    for (size_t i = 0; i < this->set_layout_bindings.at(set).size(); i++)
    {
        if (this->set_layout_bindings.at(set).at(i).binding == binding)
            index = i;
    }
    
    if (index == -1)
        throw std::runtime_error("Could not find binding in DescriptorManager::init_descriptor_write.");

    write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext = nullptr;
    write.dstSet = this->sets.at(set);
    write.dstBinding = binding;
    write.dstArrayElement = begin_element;
    write.descriptorCount = n_elements;
    write.descriptorType = this->set_layout_bindings.at(set).at(index).descriptorType;
    write.pBufferInfo = nullptr;
    write.pImageInfo = nullptr;
    write.pTexelBufferView = nullptr; 
}

void DescriptorManager::add_buffer_info(uint32_t set, uint32_t binding, uint32_t begin_element, uint32_t n_elements, const VkDescriptorBufferInfo* buffer_infos)
{
    VkWriteDescriptorSet write;
    this->init_descriptor_write(write, set, binding, begin_element, n_elements);
    write.pBufferInfo = buffer_infos;
    this->descriptor_writes.push_back(write);
}

void DescriptorManager::add_image_info(uint32_t set, uint32_t binding, uint32_t begin_element, uint32_t n_elements, const VkDescriptorImageInfo* image_infos)
{
    VkWriteDescriptorSet write;
    this->init_descriptor_write(write, set, binding, begin_element, n_elements);
    write.pImageInfo = image_infos;
    this->descriptor_writes.push_back(write);
}

void DescriptorManager::add_texel_buffer_view(uint32_t set, uint32_t binding, uint32_t begin_element, uint32_t n_elements, const VkBufferView* buffer_views)
{
    VkWriteDescriptorSet write;
    this->init_descriptor_write(write, set, binding, begin_element, n_elements);
    write.pTexelBufferView = buffer_views;
    this->descriptor_writes.push_back(write);
}


VkResult DescriptorManager::init_descriptor_layouts(void)
{
    VkDescriptorSetLayoutCreateInfo layout_ci = {};
    layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_ci.pNext = nullptr;
    layout_ci.flags = 0;

    VkResult result = VK_SUCCESS;
    for (size_t i = 0; i < this->set_layout_bindings.size(); i++)
    {
        layout_ci.bindingCount = this->set_layout_bindings.at(i).size();
        layout_ci.pBindings = this->set_layout_bindings.at(i).data();

        VkDescriptorSetLayout layout;
        if ((result = vkCreateDescriptorSetLayout(this->device, &layout_ci, nullptr, &layout)) != VK_SUCCESS)
            return result;
        this->set_layouts.push_back(layout);
    }
    return result;
}

VkResult DescriptorManager::init_descriptor_pool(void)
{
    VkDescriptorPoolCreateInfo pool_ci = {};
    pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_ci.pNext = nullptr;
    pool_ci.flags = 0;
    pool_ci.maxSets = this->set_layouts.size();
    pool_ci.poolSizeCount = this->pool_sizes.size();
    pool_ci.pPoolSizes = this->pool_sizes.data();

    return vkCreateDescriptorPool(this->device, &pool_ci, nullptr, &this->pool);
}

VkResult DescriptorManager::init_descriptor_sets(void)
{
    VkDescriptorSetAllocateInfo set_alloc_info = {};
    set_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    set_alloc_info.pNext = nullptr;
    set_alloc_info.descriptorPool = this->pool;
    set_alloc_info.descriptorSetCount = this->set_layouts.size();
    set_alloc_info.pSetLayouts = this->set_layouts.data();

    this->sets.resize(this->set_layouts.size());
    return vkAllocateDescriptorSets(this->device, &set_alloc_info, this->sets.data());
}

VkResult DescriptorManager::init(void)
{
    if (this->pool != VK_NULL_HANDLE)
        throw std::runtime_error("DescriptorManager has already been initialized.");
    if (this->device == VK_NULL_HANDLE)
        throw std::invalid_argument("Device of DescriptorManager is a VK_NULL_HANDLE, requiered from DescriptorManager::init.");

    VkResult result = VK_SUCCESS;
    if ((result = this->init_descriptor_layouts()) != VK_SUCCESS)    return result;
    if ((result = this->init_descriptor_pool()) != VK_SUCCESS)       return result;
    if ((result = this->init_descriptor_sets()) != VK_SUCCESS)       return result;

    return result;
}


void DescriptorManager::update(void)
{
    if (this->pool == VK_NULL_HANDLE)
        throw std::runtime_error("To update the descriptor sets, the DescriptorManager must be initialized.");
    if (this->device == VK_NULL_HANDLE)
        throw std::invalid_argument("Device of DescriptorManager is a VK_NULL_HANDLE, requiered from DescriptorManager::update.");

    vkUpdateDescriptorSets(this->device, this->descriptor_writes.size(), this->descriptor_writes.data(), 0, nullptr);
}


void DescriptorManager::clear(void)
{
    if (this->pool != VK_NULL_HANDLE)
    {
        if (this->device == VK_NULL_HANDLE)
            throw std::invalid_argument("Device of DescriptorManager is a VK_NULL_HANDLE, requiered from DescriptorManager::clear.");

        for (VkDescriptorSetLayout layout : this->set_layouts)
            vkDestroyDescriptorSetLayout(this->device, layout, nullptr);
        vkDestroyDescriptorPool(this->device, this->pool, nullptr);

        this->set_layouts.clear();
        this->sets.clear();
        this->pool_sizes.clear();
        this->set_layout_bindings.clear();
        this->descriptor_writes.clear();

        this->pool = VK_NULL_HANDLE;
    }
}