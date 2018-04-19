#include "stdafx.h"
#include "MemoryManager.hpp"

namespace Veldrid
{
void MemoryManager::Init(VkDevice device, VkPhysicalDevice physicalDevice)
{
    _device = device;
    _physicalDevice = physicalDevice;
}

MemoryManager::~MemoryManager()
{
    for (auto it : _allocatorsByMemoryType)
    {
        it.second->Dispose();
    }
    for (auto it : _allocatorsByMemoryTypeUnmapped)
    {
        it.second->Dispose();
    }
}

MemoryBlock MemoryManager::Allocate(
    VkPhysicalDeviceMemoryProperties memProperties,
    uint32_t memoryTypeBits,
    VkMemoryPropertyFlags flags,
    bool persistentMapped,
    VkDeviceSize size,
    VkDeviceSize alignment)
{
    _recursive_mutex.lock();
    uint32_t memoryTypeIndex = FindMemoryType(memProperties, memoryTypeBits, flags);
    ChunkAllocatorSet* allocator = GetAllocator(memoryTypeIndex, persistentMapped);
    MemoryBlock ret;
    bool result = allocator->Allocate(size, alignment, &ret);
    _recursive_mutex.unlock();

    if (!result)
    {
        throw new std::exception("Unable to allocate memory.");
    }

    return ret;
}

void MemoryManager::Free(MemoryBlock block)
{
    _recursive_mutex.lock();
    ChunkAllocatorSet* allocator = GetAllocator(block.MemoryTypeIndex, block.IsPersistentMapped());
    allocator->Free(block);
    _recursive_mutex.unlock();
}

ChunkAllocatorSet* MemoryManager::GetAllocator(uint32_t memoryTypeIndex, bool persistentMapped)
{
    ChunkAllocatorSet* ret;
    if (persistentMapped)
    {
        auto iter = _allocatorsByMemoryType.find(memoryTypeIndex);

        if (iter == _allocatorsByMemoryType.end())
        {
            ret = new ChunkAllocatorSet(_device, memoryTypeIndex, true);
            _allocatorsByMemoryType.emplace(memoryTypeIndex, ret);
        }
        else
        {
            ret = iter->second;
        }
    }
    else
    {
        auto iter = _allocatorsByMemoryTypeUnmapped.find(memoryTypeIndex);

        if (iter == _allocatorsByMemoryTypeUnmapped.end())
        {
            ret = new ChunkAllocatorSet(_device, memoryTypeIndex, false);
            _allocatorsByMemoryTypeUnmapped.emplace(memoryTypeIndex, ret);
        }
        else
        {
            ret = iter->second;
        }
    }

    return ret;
}
}