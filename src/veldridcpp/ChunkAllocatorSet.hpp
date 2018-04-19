#pragma once
#include <stdint.h>
#include "vulkan.h"
#include <vector>
#include "ChunkAllocator.hpp"

namespace Veldrid
{
class ChunkAllocatorSet
{
private:
    VkDevice _device;
    uint32_t _memoryTypeIndex;
    bool _persistentMapped;
    std::vector<ChunkAllocator> _allocators;

public:
    ChunkAllocatorSet(VkDevice device, uint32_t memoryTypeIndex, bool persistentMapped);
    bool Allocate(VkDeviceSize size, VkDeviceSize alignment, MemoryBlock* block);
    void Free(MemoryBlock block);
    void Dispose()
    {
        for (ChunkAllocator& allocator : _allocators)
        {
            allocator.Dispose();
        }

        delete this;
    }
};
}