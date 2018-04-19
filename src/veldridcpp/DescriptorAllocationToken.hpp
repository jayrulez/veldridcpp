#pragma once
#include "vulkan.h"

namespace Veldrid
{
struct DescriptorAllocationToken
{
    VkDescriptorSet Set;
    VkDescriptorPool Pool;
    DescriptorAllocationToken() {}
    DescriptorAllocationToken(VkDescriptorSet set, VkDescriptorPool pool)
    {
        Set = set;
        Pool = pool;
    }
};
}