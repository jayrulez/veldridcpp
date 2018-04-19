#pragma once

#include "PixelFormat.hpp"
#include "GraphicsDevice.hpp"
#include "MemoryBlock.hpp"
#include "TextureDescription.hpp"
#include "TextureUsage.hpp"
#include "TextureType.hpp"
#include "TextureSampleCount.hpp"
#include "vulkan.h"
#include "stdint.h"

namespace Veldrid
{
class Texture
{
public:
    Texture(GraphicsDevice* gd, const TextureDescription& description);
    // Used to construct Swapchain textures.
    Texture(
        GraphicsDevice* gd,
        uint32_t width,
        uint32_t height,
        uint32_t mipLevels,
        uint32_t arrayLayers,
        VkFormat vkFormat,
        TextureUsage usage,
        TextureSampleCount sampleCount,
        VkImage existingImage);
    ~Texture();

    inline uint32_t GetWidth() const { return _width; }
    inline uint32_t GetHeight() const { return _height; }
    inline uint32_t GetDepth() const { return _depth; }
    inline uint32_t GetMipLevels() const { return _mipLevels; }
    inline uint32_t GetArrayLayers() const { return _arrayLayers; }
    inline PixelFormat GetFormat() const { return _format; }
    inline TextureUsage GetUsage() const { return _usage; }
    inline TextureSampleCount GetSampleCount() const { return _sampleCount; }
    inline const MemoryBlock& GetMemory() const { return _memoryBlock; }
    inline VkImage GetOptimalImage() const { return _optimalImage; }
    inline VkBuffer GetStagingBuffer() const { return _stagingBuffer; }
    inline VkFormat GetVkFormat() const { return _vkFormat; }
    inline VkSampleCountFlagBits GetVkSampleCount() const { return (VkSampleCountFlagBits)_vkSampleCount; }

    VkSubresourceLayout GetSubresourceLayout(uint32_t subresource) const;
    uint32_t CalculateSubresource(uint32_t mipLevel, uint32_t arrayLayer) const { return arrayLayer * _mipLevels + mipLevel; }
    void SetStagingDimensions(uint32_t width, uint32_t height, uint32_t depth, PixelFormat format);
    void TransitionImageLayout(
        VkCommandBuffer cb,
        uint32_t baseMipLevel,
        uint32_t levelCount,
        uint32_t baseArrayLayer,
        uint32_t layerCount,
        VkImageLayout newLayout);
    void SetImageLayout(uint32_t mipLevel, uint32_t arrayLayer, VkImageLayout layout);
    void ClearIfRenderTarget();

    VdResult GetDescription(TextureDescription* description);

private:
    GraphicsDevice * _gd;
    VkImage _optimalImage;
    MemoryBlock _memoryBlock;
    VkBuffer _stagingBuffer;
    PixelFormat _format; // Static for regular images -- may change for shared staging images
    uint32_t _actualImageArrayLayers;
    uint32_t _mipLevels;
    uint32_t _arrayLayers;
    TextureUsage _usage;
    TextureType _type;
    TextureSampleCount _sampleCount;
    VkSampleCountFlags _vkSampleCount;
    VkFormat _vkFormat;
    VkImageLayout* _imageLayouts;
    bool _isImageOwned; // False for Swapchain images.

    // Immutable except for shared staging Textures.
    uint32_t _width;
    uint32_t _height;
    uint32_t _depth;
};
}