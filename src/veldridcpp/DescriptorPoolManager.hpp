#pragma once
#include "stdint.h"
#include "DescriptorAllocationToken.hpp"
#include "DescriptorResourceCounts.hpp"
#include "GraphicsDevice.hpp"
#include "vulkan.h"
#include <mutex>
#include <vector>

namespace Veldrid
{
class DescriptorPoolManager
{
    struct PoolInfo
    {
        VkDescriptorPool Pool;
        uint32_t RemainingSets;
        uint32_t UniformBufferCount;
        uint32_t SampledImageCount;
        uint32_t SamplerCount;
        uint32_t StorageBufferCount;
        uint32_t StorageImageCount;

        PoolInfo(VkDescriptorPool pool, uint32_t totalSets, uint32_t descriptorCount);
        bool Allocate(const DescriptorResourceCounts& counts);
        void Free(VkDevice device, const DescriptorAllocationToken& token, const DescriptorResourceCounts& counts);
    };

public:
    DescriptorPoolManager(GraphicsDevice* gd);
    ~DescriptorPoolManager();
    DescriptorAllocationToken Allocate(const DescriptorResourceCounts& counts, VkDescriptorSetLayout setLayout);
    void Free(const DescriptorAllocationToken& token, const DescriptorResourceCounts& counts);

private:
    GraphicsDevice * _gd;
    std::vector<PoolInfo> _pools;
    std::mutex _lock;

    PoolInfo CreateNewPool();
    VkDescriptorPool GetPool(DescriptorResourceCounts counts);
};
}