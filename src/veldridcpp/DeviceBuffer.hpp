#pragma once
#include "BufferDescription.hpp"
#include "GraphicsDevice.hpp"
#include "vulkan.h"
#include <stdint.h>

namespace Veldrid
{
class DeviceBuffer
{
public:
    DeviceBuffer(GraphicsDevice* const device, const BufferDescription& description);
    void Destroy() const;

    uint32_t GetSizeInBytes() const { return _size; }
    BufferUsage GetUsage() const { return _usage; }
    MemoryBlock GetMemory() const { return _memory; }
    VkBuffer GetVkBuffer() const { return _vkBuffer; }

private:
    GraphicsDevice * const _gd;
    VkBuffer _vkBuffer;
    uint32_t _size;
    BufferUsage _usage;
    MemoryBlock _memory;
};
}