#pragma once
#include "GraphicsDevice.hpp"
#include "DescriptorResourceCounts.hpp"
#include "ResourceLayoutDescription.hpp"
#include "VkFormats.hpp"
#include "vulkan.h"
#include <vector>

namespace Veldrid
{
class ResourceLayout
{
public:
    ResourceLayout(GraphicsDevice* gd, const ResourceLayoutDescription& description);
    ~ResourceLayout();
    VkDescriptorSetLayout DescriptorSetLayout() const { return _dsl; }
    std::vector<VkDescriptorType> DescriptorTypes() const { return _descriptorTypes; }
    DescriptorResourceCounts DescriptorCounts() const { return _descriptorResourceCounts; }

private:
    GraphicsDevice * _gd;
    VkDescriptorSetLayout _dsl;
    std::vector<VkDescriptorType> _descriptorTypes;
    DescriptorResourceCounts _descriptorResourceCounts;
};
}
