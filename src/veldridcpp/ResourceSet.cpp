#include "stdafx.h"
#include "ResourceSet.hpp"
#include "TextureView.hpp"

namespace Veldrid
{
ResourceSet::ResourceSet(GraphicsDevice * gd, const ResourceSetDescription & description)
{
    _gd = gd;
    ResourceLayout* vkLayout = description.Layout;

    VkDescriptorSetLayout dsl = vkLayout->DescriptorSetLayout();
    _descriptorCounts = vkLayout->DescriptorCounts();
    _descriptorAllocationToken = _gd->GetDescriptorPoolManager().Allocate(_descriptorCounts, dsl);

    const InteropArray<void*>& boundResources = description.BoundResources;
    uint32_t descriptorWriteCount = boundResources.Count;

    std::vector<VkWriteDescriptorSet> descriptorWrites(descriptorWriteCount);
    std::vector<VkDescriptorBufferInfo> bufferInfos(descriptorWriteCount);
    std::vector<VkDescriptorImageInfo> imageInfos(descriptorWriteCount);

    for (uint32_t i = 0; i < descriptorWriteCount; i++)
    {
        VkDescriptorType type = vkLayout->DescriptorTypes()[i];

        descriptorWrites[i].sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[i].descriptorCount = 1;
        descriptorWrites[i].descriptorType = type;
        descriptorWrites[i].dstBinding = i;
        descriptorWrites[i].dstSet = _descriptorAllocationToken.Set;

        if (type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER || type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
        {
            DeviceBuffer* vkBuffer = (DeviceBuffer*)boundResources[i];
            bufferInfos[i].buffer = vkBuffer->GetVkBuffer();
            bufferInfos[i].range = vkBuffer->GetSizeInBytes();
            descriptorWrites[i].pBufferInfo = &bufferInfos[i];
        }
        else if (type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
        {
            TextureView* textureView = (TextureView*)boundResources[i];
            imageInfos[i].imageView = textureView->GetVkImageView();
            imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            descriptorWrites[i].pImageInfo = &imageInfos[i];
        }
        else if (type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
        {
            TextureView* textureView = (TextureView*)boundResources[i];
            imageInfos[i].imageView = textureView->GetVkImageView();
            imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            descriptorWrites[i].pImageInfo = &imageInfos[i];
        }
        else if (type == VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLER)
        {
            Sampler* sampler = (Sampler*)boundResources[i];
            imageInfos[i].sampler = sampler->GetVkSampler();
            descriptorWrites[i].pImageInfo = &imageInfos[i];
        }
    }

    vkUpdateDescriptorSets(_gd->GetVkDevice(), descriptorWriteCount, descriptorWrites.data(), 0, nullptr);
}

VD_EXPORT void VdResourceSet_Dispose(ResourceSet* rs)
{
    delete rs;
}
}
