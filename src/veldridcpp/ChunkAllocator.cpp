#include "stdafx.h"
#include "ChunkAllocator.hpp"
#include "VulkanUtil.hpp"

namespace Veldrid
{
ChunkAllocator::ChunkAllocator(VkDevice device, uint32_t memoryTypeIndex, bool persistentMapped)
{
    _device = device;
    _memoryTypeIndex = memoryTypeIndex;
    _persistentMapped = persistentMapped;
    _totalMemorySize = persistentMapped ? PersistentMappedChunkSize : UnmappedChunkSize;

    VkMemoryAllocateInfo memoryAI = {};
    memoryAI.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAI.allocationSize = _totalMemorySize;
    memoryAI.memoryTypeIndex = _memoryTypeIndex;
    CheckResult(vkAllocateMemory(_device, &memoryAI, nullptr, &_memory));

    void* mappedPtr = nullptr;
    if (persistentMapped)
    {
        CheckResult(vkMapMemory(_device, _memory, 0, _totalMemorySize, 0, &mappedPtr));
    }

    _freeBlocks.emplace_back(_memory, 0, _totalMemorySize, _memoryTypeIndex, mappedPtr);
}

bool ChunkAllocator::Allocate(VkDeviceSize size, VkDeviceSize alignment, MemoryBlock* block)
{
    for (int i = 0; i < _freeBlocks.size(); i++)
    {
        MemoryBlock freeBlock = _freeBlocks[i];
        VkDeviceSize alignedBlockSize = freeBlock.Size;
        if (freeBlock.Offset % alignment != 0)
        {
            VkDeviceSize alignmentCorrection = (alignment - freeBlock.Offset % alignment);
            if (alignedBlockSize <= alignmentCorrection)
            {
                continue;
            }
            alignedBlockSize -= alignmentCorrection;
        }

        if (alignedBlockSize >= size) // Valid match -- split it and return.
        {
            _freeBlocks.erase(_freeBlocks.begin() + i);

            freeBlock.Size = alignedBlockSize;
            if ((freeBlock.Offset % alignment) != 0)
            {
                freeBlock.Offset += alignment - (freeBlock.Offset % alignment);
            }

            *block = freeBlock;

            if (alignedBlockSize != size)
            {
                _freeBlocks.emplace_back(
                    freeBlock.DeviceMemory,
                    freeBlock.Offset + size,
                    freeBlock.Size - size,
                    _memoryTypeIndex,
                    freeBlock.BaseMappedPointer);
                *block = freeBlock;
                block->Size = size;
            }

            _totalAllocatedBytes += alignedBlockSize;
            return true;
        }
    }

    block = nullptr;
    return false;
}

void ChunkAllocator::Free(MemoryBlock block)
{
    _freeBlocks.push_back(block);
}

void ChunkAllocator::Dispose() { vkFreeMemory(_device, _memory, nullptr); }
}