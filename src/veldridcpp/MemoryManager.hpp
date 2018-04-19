#pragma once
#include <mutex>
#include "vulkan.h"
#include "VulkanUtil.hpp"
#include "ChunkAllocatorSet.hpp"
#include "ChunkAllocator.hpp"
#include "MemoryBlock.hpp"

namespace Veldrid
{
class MemoryManager
{
public:
    void Init(VkDevice device, VkPhysicalDevice physicalDevice);
    ~MemoryManager();
    MemoryBlock Allocate(
        VkPhysicalDeviceMemoryProperties memProperties,
        uint32_t memoryTypeBits,
        VkMemoryPropertyFlags flags,
        bool persistentMapped,
        VkDeviceSize size,
        VkDeviceSize alignment);
    void Free(MemoryBlock block);

private:
    VkDevice _device;
    VkPhysicalDevice _physicalDevice;
    std::recursive_mutex _recursive_mutex;
    std::unordered_map<uint32_t, ChunkAllocatorSet*> _allocatorsByMemoryTypeUnmapped;
    std::unordered_map<uint32_t, ChunkAllocatorSet*> _allocatorsByMemoryType;

    ChunkAllocatorSet* GetAllocator(uint32_t memoryTypeIndex, bool persistentMapped);
};
}