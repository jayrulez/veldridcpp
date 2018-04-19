#include "stdafx.h"
#include "DeviceBuffer.hpp"
#include "BufferUsage.hpp"
#include "GraphicsDevice.hpp"
#include "VulkanUtil.hpp"
#include "vulkan.h"
#include <stdint.h>

namespace Veldrid
{
DeviceBuffer::DeviceBuffer(GraphicsDevice* const device, const BufferDescription& description)
    : _gd(device)
{
    VkDevice vkDevice = _gd->GetVkDevice();
    _size = description.SizeInBytes;
    _usage = description.Usage;
    VkBufferUsageFlags vkUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    if ((_usage & BufferUsage::VertexBuffer) == BufferUsage::VertexBuffer)
    {
        vkUsage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    }
    if ((_usage & BufferUsage::IndexBuffer) == BufferUsage::IndexBuffer)
    {
        vkUsage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    }
    if ((_usage & BufferUsage::UniformBuffer) == BufferUsage::UniformBuffer)
    {
        vkUsage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    }
    if ((_usage & BufferUsage::StructuredBufferReadOnly) == BufferUsage::StructuredBufferReadOnly
        || (_usage & BufferUsage::StructuredBufferReadWrite) == BufferUsage::StructuredBufferReadWrite)
    {
        vkUsage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    }
    if ((_usage & BufferUsage::IndirectBuffer) == BufferUsage::IndirectBuffer)
    {
        vkUsage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    }

    auto bufferCI = VkBufferCreateInfo();
    bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCI.size = description.SizeInBytes;
    bufferCI.usage = vkUsage;
    CheckResult(vkCreateBuffer(vkDevice, &bufferCI, nullptr, &_vkBuffer));

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(vkDevice, _vkBuffer, &memReqs);

    bool hostVisible = (_usage & BufferUsage::Dynamic) == BufferUsage::Dynamic
        || (_usage & BufferUsage::Staging) == BufferUsage::Staging;

    VkMemoryPropertyFlags memoryPropertyFlags =
        hostVisible
        ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    MemoryBlock memoryToken = _gd->GetMemoryManager().Allocate(
        _gd->GetPhysicalDeviceMemProperties(),
        memReqs.memoryTypeBits,
        memoryPropertyFlags,
        hostVisible,
        memReqs.size,
        memReqs.alignment);
    _memory = memoryToken;
    CheckResult(vkBindBufferMemory(_gd->GetVkDevice(), _vkBuffer, _memory.DeviceMemory, _memory.Offset));
}

void DeviceBuffer::Destroy() const
{
    vkDestroyBuffer(_gd->GetVkDevice(), _vkBuffer, nullptr);
    delete this;
}

VD_EXPORT void VdDeviceBuffer_Dispose(DeviceBuffer* buffer) { buffer->Destroy(); }

}