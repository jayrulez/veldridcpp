#pragma once
#include <stdint.h>
#include "vulkan.h"

namespace Veldrid
{
struct MemoryBlock
{
    uint32_t MemoryTypeIndex;
    VkDeviceMemory DeviceMemory;
    void* BaseMappedPointer;
    VkDeviceSize Offset;
    VkDeviceSize Size;

    uint8_t* BlockMappedPointer() const { return (uint8_t*)BaseMappedPointer + Offset; }
    bool IsPersistentMapped() const { return BaseMappedPointer != nullptr; }

    MemoryBlock() {}

    MemoryBlock(VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, uint32_t memoryTypeIndex, void* baseMappedPtr)
    {
        DeviceMemory = memory;
        Offset = offset;
        Size = size;
        MemoryTypeIndex = memoryTypeIndex;
        BaseMappedPointer = baseMappedPtr;
    }
};
}