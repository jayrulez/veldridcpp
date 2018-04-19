#include "stdafx.h"
#include "MemoryManager.hpp"
#include "ChunkAllocatorSet.hpp"

namespace Veldrid
{
ChunkAllocatorSet::ChunkAllocatorSet(VkDevice device, uint32_t memoryTypeIndex, bool persistentMapped)
{
    _device = device;
    _memoryTypeIndex = memoryTypeIndex;
    _persistentMapped = persistentMapped;
}

bool ChunkAllocatorSet::Allocate(VkDeviceSize size, VkDeviceSize alignment, MemoryBlock * block)
{
    for (ChunkAllocator& allocator : _allocators)
    {
        if (allocator.Allocate(size, alignment, block))
        {
            return true;
        }
    }

    ChunkAllocator& newAllocator = _allocators.emplace_back(_device, _memoryTypeIndex, _persistentMapped);
    return newAllocator.Allocate(size, alignment, block);
}

void ChunkAllocatorSet::Free(MemoryBlock block)
{
    for (ChunkAllocator& chunk : _allocators)
    {
        if (chunk.Memory() == block.DeviceMemory)
        {
            chunk.Free(block);
        }
    }
}
}