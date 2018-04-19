#pragma once
#include "vulkan.h"
#include "GraphicsDevice.hpp"

namespace Veldrid
{
class Fence
{
public:
    Fence(GraphicsDevice* gd)
    {
        _gd = gd;
        VkFenceCreateInfo fenceCI = {};
        fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        CheckResult(vkCreateFence(_gd->GetVkDevice(), &fenceCI, nullptr, &_vkFence));
    }

    VkFence GetVkFence() { return _vkFence; }

private:
    GraphicsDevice * _gd;
    VkFence _vkFence;
};
}