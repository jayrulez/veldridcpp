#include "stdafx.h"
#include "ResourceLayout.hpp"

namespace Veldrid
{
ResourceLayout::ResourceLayout(GraphicsDevice * gd, const ResourceLayoutDescription & description)
{
    _gd = gd;
    VkDescriptorSetLayoutCreateInfo dslCI = {};
    dslCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    const InteropArray<ResourceLayoutElementDescription>& elements = description.Elements;
    _descriptorTypes.resize(elements.Count);
    std::vector<VkDescriptorSetLayoutBinding> bindings(elements.Count);

    uint32_t uniformBufferCount = 0;
    uint32_t sampledImageCount = 0;
    uint32_t samplerCount = 0;
    uint32_t storageBufferCount = 0;
    uint32_t storageImageCount = 0;

    for (uint32_t i = 0; i < elements.Count; i++)
    {
        bindings[i].binding = i;
        bindings[i].descriptorCount = 1;
        VkDescriptorType descriptorType = VdToVkDescriptorType(elements[i].Kind);
        bindings[i].descriptorType = descriptorType;
        bindings[i].stageFlags = VdToVkShaderStages(elements[i].Stages);

        _descriptorTypes[i] = descriptorType;

        switch (descriptorType)
        {
        case VK_DESCRIPTOR_TYPE_SAMPLER:
            samplerCount += 1;
            break;
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            sampledImageCount += 1;
            break;
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            storageImageCount += 1;
            break;
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            uniformBufferCount += 1;
            break;
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            storageBufferCount += 1;
            break;
        }
    }

    _descriptorResourceCounts.UniformBufferCount = uniformBufferCount;
    _descriptorResourceCounts.SampledImageCount = sampledImageCount;
    _descriptorResourceCounts.SamplerCount = samplerCount;
    _descriptorResourceCounts.StorageBufferCount = storageBufferCount;
    _descriptorResourceCounts.StorageImageCount = storageImageCount;

    dslCI.bindingCount = elements.Count;
    dslCI.pBindings = bindings.data();

    VkResult result = vkCreateDescriptorSetLayout(_gd->GetVkDevice(), &dslCI, nullptr, &_dsl);
    CheckResult(result);
}

ResourceLayout::~ResourceLayout()
{
    vkDestroyDescriptorSetLayout(_gd->GetVkDevice(), _dsl, nullptr);
}

VD_EXPORT void VdResourceLayout_Dispose(ResourceLayout* resourceLayout)
{
    delete resourceLayout;
}
}
