#include "stdafx.h"
#include "DescriptorPoolManager.hpp"

namespace Veldrid
{
DescriptorPoolManager::PoolInfo::PoolInfo(VkDescriptorPool pool, uint32_t totalSets, uint32_t descriptorCount)
{
    Pool = pool;
    RemainingSets = totalSets;
    UniformBufferCount = descriptorCount;
    SampledImageCount = descriptorCount;
    SamplerCount = descriptorCount;
    StorageBufferCount = descriptorCount;
    StorageImageCount = descriptorCount;
}
bool DescriptorPoolManager::PoolInfo::Allocate(const DescriptorResourceCounts & counts)
{
    if (RemainingSets > 0
        && UniformBufferCount >= counts.UniformBufferCount
        && SampledImageCount >= counts.SampledImageCount
        && SamplerCount >= counts.SamplerCount
        && StorageBufferCount >= counts.SamplerCount
        && StorageImageCount >= counts.StorageImageCount)
    {
        RemainingSets -= 1;
        UniformBufferCount -= counts.UniformBufferCount;
        SampledImageCount -= counts.SampledImageCount;
        SamplerCount -= counts.SamplerCount;
        StorageBufferCount -= counts.StorageBufferCount;
        StorageImageCount -= counts.StorageImageCount;
        return true;
    }
    else
    {
        return false;
    }
}
void DescriptorPoolManager::PoolInfo::Free(VkDevice device, const DescriptorAllocationToken & token, const DescriptorResourceCounts & counts)
{
    VkDescriptorSet set = token.Set;
    vkFreeDescriptorSets(device, Pool, 1, &set);

    RemainingSets += 1;

    UniformBufferCount += counts.UniformBufferCount;
    SampledImageCount += counts.SampledImageCount;
    SamplerCount += counts.SamplerCount;
    StorageBufferCount += counts.StorageBufferCount;
    StorageImageCount += counts.StorageImageCount;
}
DescriptorPoolManager::DescriptorPoolManager(GraphicsDevice * gd)
{
    _gd = gd;
    _pools.push_back(CreateNewPool());
}
DescriptorPoolManager::~DescriptorPoolManager()
{
    for (auto& poolInfo : _pools)
    {
        vkDestroyDescriptorPool(_gd->GetVkDevice(), poolInfo.Pool, nullptr);
    }
}
DescriptorAllocationToken DescriptorPoolManager::Allocate(const DescriptorResourceCounts & counts, VkDescriptorSetLayout setLayout)
{
    VkDescriptorPool pool = GetPool(counts);
    VkDescriptorSetAllocateInfo dsAI = {};
    dsAI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    dsAI.descriptorSetCount = 1;
    dsAI.pSetLayouts = &setLayout;
    dsAI.descriptorPool = pool;
    VkDescriptorSet set;
    VkResult result = vkAllocateDescriptorSets(_gd->GetVkDevice(), &dsAI, &set);
    CheckResult(result);

    return DescriptorAllocationToken(set, pool);
}
void DescriptorPoolManager::Free(const DescriptorAllocationToken & token, const DescriptorResourceCounts & counts)
{
    _lock.lock();
    for (auto& poolInfo : _pools)
    {
        if (poolInfo.Pool == token.Pool)
        {
            poolInfo.Free(_gd->GetVkDevice(), token, counts);
            break;
        }
    }
    _lock.unlock();
}
DescriptorPoolManager::PoolInfo DescriptorPoolManager::CreateNewPool()
{
    uint32_t totalSets = 1000;
    uint32_t descriptorCount = 100;
    uint32_t poolSizeCount = 5;
    std::vector<VkDescriptorPoolSize> sizes(poolSizeCount);
    sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    sizes[0].descriptorCount = descriptorCount;
    sizes[1].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    sizes[1].descriptorCount = descriptorCount;
    sizes[2].type = VK_DESCRIPTOR_TYPE_SAMPLER;
    sizes[2].descriptorCount = descriptorCount;
    sizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    sizes[3].descriptorCount = descriptorCount;
    sizes[4].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    sizes[4].descriptorCount = descriptorCount;

    VkDescriptorPoolCreateInfo poolCI = {};
    poolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolCI.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolCI.maxSets = totalSets;
    poolCI.pPoolSizes = sizes.data();
    poolCI.poolSizeCount = poolSizeCount;

    VkDescriptorPool descriptorPool;
    VkResult result = vkCreateDescriptorPool(_gd->GetVkDevice(), &poolCI, nullptr, &descriptorPool);
    CheckResult(result);

    return PoolInfo(descriptorPool, totalSets, descriptorCount);
}
VkDescriptorPool DescriptorPoolManager::GetPool(DescriptorResourceCounts counts)
{
    _lock.lock();
    for (auto& poolInfo : _pools)
    {
        if (poolInfo.Allocate(counts))
        {
            _lock.unlock();
            return poolInfo.Pool;
        }
    }

    PoolInfo newPool = CreateNewPool();
    _pools.push_back(newPool);
    bool result = newPool.Allocate(counts);
    VdAssert(result);
    _lock.unlock();
    return newPool.Pool;
}
}
