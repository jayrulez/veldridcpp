#pragma once
#include "stdint.h"
#include "DescriptorResourceCounts.hpp"
#include "DeviceBuffer.hpp"
#include "GraphicsDevice.hpp"
#include "ResourceSetDescription.hpp"
#include "Sampler.hpp"
#include "vulkan.h"
#include "DescriptorPoolManager.hpp"

namespace Veldrid
{
class ResourceSet
{
public:
    ResourceSet(GraphicsDevice* gd, const ResourceSetDescription& description);
    VkDescriptorSet DescriptorSet() const { return _descriptorAllocationToken.Set; };

private:
    GraphicsDevice * _gd;
    DescriptorResourceCounts _descriptorCounts;
    DescriptorAllocationToken _descriptorAllocationToken;
};
}