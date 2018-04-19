#pragma once
#include "vulkan.h"
#include "GraphicsDevice.hpp"
#include "SamplerDescription.hpp"
#include "VulkanUtil.hpp"

namespace Veldrid
{
class Sampler
{
public:
    Sampler(GraphicsDevice* gd, const SamplerDescription& description);
    ~Sampler();
    VkSampler GetVkSampler() const { return _vkSampler; }

private:
    GraphicsDevice* _gd;
    VkSampler _vkSampler;
};
}