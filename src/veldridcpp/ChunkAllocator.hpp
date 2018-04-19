#pragma once
#include <stdint.h>
#include "vulkan.h"
#include <vector>
#include "MemoryBlock.hpp"
namespace Veldrid
{
class ChunkAllocator
{
private:
    static const VkDeviceSize PersistentMappedChunkSize = 1024 * 1024 * 64;
    static const VkDeviceSize UnmappedChunkSize = 1024 * 1024 * 256;
    VkDevice _device;
    uint32_t _memoryTypeIndex;
    bool _persistentMapped;
    std::vector<MemoryBlock> _freeBlocks;
    VkDeviceMemory _memory;
    VkDeviceSize _totalMemorySize;
    VkDeviceSize _totalAllocatedBytes = 0;

public:
    ChunkAllocator(VkDevice device, uint32_t memoryTypeIndex, bool persistentMapped);
    VkDeviceMemory Memory() { return  _memory; }
    bool Allocate(VkDeviceSize size, VkDeviceSize alignment, MemoryBlock* block);
    void Free(MemoryBlock block);
    void Dispose();
};
}
