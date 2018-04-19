#include "stdafx.h"
#include "Texture.hpp"
#include "FormatHelpers.hpp"
#include "Util.hpp"
#include "VkFormats.hpp"
#include "VeldridConfig.hpp"

namespace Veldrid
{
Texture::Texture(GraphicsDevice* gd, const TextureDescription& description)
{
    _gd = gd;
    _width = description.Width;
    _height = description.Height;
    _depth = description.Depth;
    _mipLevels = description.MipLevels;
    _arrayLayers = description.ArrayLayers;
    bool isCubemap = ((description.Usage) & TextureUsage::Cubemap) == TextureUsage::Cubemap;
    _actualImageArrayLayers = isCubemap
        ? 6 * _arrayLayers
        : _arrayLayers;
    _format = description.Format;
    _usage = description.Usage;
    _type = description.Type;
    _sampleCount = description.SampleCount;
    _vkSampleCount = VdToVkSampleCount(_sampleCount);
    _vkFormat = VdToVkPixelFormat(_format, (description.Usage & TextureUsage::DepthStencil) == TextureUsage::DepthStencil);

    bool isStaging = (_usage & TextureUsage::Staging) == TextureUsage::Staging;

    if (!isStaging)
    {
        VkImageCreateInfo imageCI = {};
        imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCI.mipLevels = _mipLevels;
        imageCI.arrayLayers = _actualImageArrayLayers;
        imageCI.imageType = VdToVkTextureType(_type);
        imageCI.extent.width = _width;
        imageCI.extent.height = _height;
        imageCI.extent.depth = _depth;
        imageCI.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
        imageCI.usage = VdToVkTextureUsage(_usage);
        imageCI.tiling = isStaging ? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL;
        imageCI.format = _vkFormat;

        imageCI.samples = static_cast<VkSampleCountFlagBits>(_vkSampleCount);
        if (isCubemap)
        {
            imageCI.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        }

        uint32_t subresourceCount = _mipLevels * _actualImageArrayLayers * _depth;
        VkResult result = vkCreateImage(_gd->GetVkDevice(), &imageCI, nullptr, &_optimalImage);
        CheckResult(result);

        VkMemoryRequirements memoryRequirements;
        vkGetImageMemoryRequirements(_gd->GetVkDevice(), _optimalImage, &memoryRequirements);

        MemoryBlock memoryToken = _gd->GetMemoryManager().Allocate(
            _gd->GetPhysicalDeviceMemProperties(),
            memoryRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            false,
            memoryRequirements.size,
            memoryRequirements.alignment);
        _memoryBlock = memoryToken;
        result = vkBindImageMemory(_gd->GetVkDevice(), _optimalImage, _memoryBlock.DeviceMemory, _memoryBlock.Offset);
        CheckResult(result);

        _imageLayouts = new VkImageLayout[subresourceCount];
        for (uint32_t i = 0; i < subresourceCount; i++)
        {
            _imageLayouts[i] = VK_IMAGE_LAYOUT_PREINITIALIZED;
        }
    }
    else // isStaging
    {
        uint32_t depthPitch = GetDepthPitch(
            GetRowPitch(_width, _format),
            _height,
            _format);
        uint32_t stagingSize = depthPitch * _depth;
        for (uint32_t level = 1; level < _mipLevels; level++)
        {
            uint32_t mipWidth, mipHeight, mipDepth;
            GetMipDimensions(this, level, &mipWidth, &mipHeight, &mipDepth);

            depthPitch = GetDepthPitch(
                GetRowPitch(mipWidth, _format),
                mipHeight,
                _format);

            stagingSize += depthPitch * mipDepth;
        }
        stagingSize *= _arrayLayers;

        VkBufferCreateInfo bufferCI = {};
        bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        bufferCI.size = stagingSize;
        VkResult result = vkCreateBuffer(_gd->GetVkDevice(), &bufferCI, nullptr, &_stagingBuffer);
        CheckResult(result);
        VkMemoryRequirements bufferMemReqs;
        vkGetBufferMemoryRequirements(_gd->GetVkDevice(), _stagingBuffer, &bufferMemReqs);
        _memoryBlock = _gd->GetMemoryManager().Allocate(
            _gd->GetPhysicalDeviceMemProperties(),
            bufferMemReqs.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            true,
            bufferMemReqs.size,
            bufferMemReqs.alignment);

        result = vkBindBufferMemory(_gd->GetVkDevice(), _stagingBuffer, _memoryBlock.DeviceMemory, _memoryBlock.Offset);
        CheckResult(result);
    }

    ClearIfRenderTarget();
}

void Texture::ClearIfRenderTarget()
{
    // If the image is going to be used as a render target, we need to clear the data before its first use.
    if (HasFlag(_usage, TextureUsage::RenderTarget))
    {
        VkClearColorValue color;
        color.uint32[0] = 0u;
        color.uint32[1] = 0u;
        color.uint32[2] = 0u;
        color.uint32[3] = 0u;
        _gd->ClearColorTexture(this, color);
    }
    if (HasFlag(_usage, TextureUsage::DepthStencil))
    {
        VkClearDepthStencilValue value;
        value.depth = 0;
        value.stencil = 0;
        _gd->ClearDepthTexture(this, value);
    }
}

VdResult Texture::GetDescription(TextureDescription* description)
{
    description->Width = _width;
    description->Height = _height;
    description->Depth = _depth;
    description->MipLevels = _mipLevels;
    description->ArrayLayers = _arrayLayers;
    description->Format = _format;
    description->Usage = _usage;
    description->Type = _type;
    description->SampleCount = _sampleCount;
    return VdResult::Success;
}

Texture::Texture(
    GraphicsDevice* gd,
    uint32_t width,
    uint32_t height,
    uint32_t mipLevels,
    uint32_t arrayLayers,
    VkFormat vkFormat,
    TextureUsage usage,
    TextureSampleCount sampleCount,
    VkImage existingImage)
{
    VdAssert(width > 0 && height > 0);
    _gd = gd;
    _mipLevels = mipLevels;
    _width = width;
    _height = height;
    _depth = 1;
    _vkFormat = vkFormat;
    _format = VkToVdPixelFormat(_vkFormat);
    _arrayLayers = arrayLayers;
    _usage = usage;
    _sampleCount = sampleCount;
    _vkSampleCount = VdToVkSampleCount(sampleCount);
    _optimalImage = existingImage;
    _imageLayouts = new VkImageLayout[1];
    _imageLayouts[0] = VK_IMAGE_LAYOUT_PREINITIALIZED;
    _type = TextureType::Texture2D;
    _isImageOwned = false;

    ClearIfRenderTarget();
}

Texture::~Texture()
{
    bool isStaging = (_usage & TextureUsage::Staging) == TextureUsage::Staging;
    if (isStaging)
    {
        vkDestroyBuffer(_gd->GetVkDevice(), _stagingBuffer, nullptr);
    }
    else if (_isImageOwned)
    {
        vkDestroyImage(_gd->GetVkDevice(), _optimalImage, nullptr);
    }

    if (_memoryBlock.DeviceMemory != VK_NULL_HANDLE)
    {
        _gd->GetMemoryManager().Free(_memoryBlock);
    }

    delete _imageLayouts;
}

VkSubresourceLayout Texture::GetSubresourceLayout(uint32_t subresource) const
{
    bool staging = _stagingBuffer != VK_NULL_HANDLE;
    uint32_t mipLevel, arrayLayer;
    GetMipLevelAndArrayLayer(this, subresource, &mipLevel, &arrayLayer);
    if (!staging)
    {
        VkImageAspectFlags aspect = (_usage & TextureUsage::DepthStencil) == TextureUsage::DepthStencil
            ? (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)
            : VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
        VkImageSubresource imageSubresource;
        imageSubresource.arrayLayer = arrayLayer;
        imageSubresource.mipLevel = mipLevel;
        imageSubresource.aspectMask = aspect;

        VkSubresourceLayout layout;
        vkGetImageSubresourceLayout(_gd->GetVkDevice(), _optimalImage, &imageSubresource, &layout);
        return layout;
    }
    else
    {
        uint32_t blockSize = IsCompressedFormat(_format) ? 4u : 1u;
        uint32_t mipWidth, mipHeight, mipDepth;
        GetMipDimensions(this, mipLevel, &mipWidth, &mipHeight, &mipDepth);
        uint32_t rowPitch = GetRowPitch(mipWidth, _format);
        uint32_t depthPitch = GetDepthPitch(rowPitch, mipHeight, _format);

        VkSubresourceLayout layout;
        layout.rowPitch = rowPitch;
        layout.depthPitch = depthPitch;
        layout.arrayPitch = depthPitch;
        layout.size = depthPitch * mipDepth;

        layout.offset = ComputeSubresourceOffset(this, mipLevel, arrayLayer);

        return layout;
    }
}

void Texture::SetStagingDimensions(uint32_t width, uint32_t height, uint32_t depth, PixelFormat format)
{
    _width = width;
    _height = height;
    _depth = depth;
    _format = format;
}

void Texture::TransitionImageLayout(VkCommandBuffer cb, uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount, VkImageLayout newLayout)
{
    if (_stagingBuffer != VK_NULL_HANDLE)
    {
        return;
    }

    VkImageLayout oldLayout = _imageLayouts[CalculateSubresource(baseMipLevel, baseArrayLayer)];
    if (oldLayout != newLayout)
    {
        VkImageAspectFlags aspectMask;
        if (HasFlag(_usage, TextureUsage::DepthStencil))
        {
            aspectMask = IsStencilFormat(_format)
                ? aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT
                : aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        }
        else
        {
            aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;;
        }
        VulkanUtil_TransitionImageLayout(
            cb,
            _optimalImage,
            baseMipLevel,
            levelCount,
            baseArrayLayer,
            layerCount,
            aspectMask,
            _imageLayouts[CalculateSubresource(baseMipLevel, baseArrayLayer)],
            newLayout);

        for (uint32_t level = 0; level < levelCount; level++)
        {
            for (uint32_t layer = 0; layer < layerCount; layer++)
            {
                _imageLayouts[CalculateSubresource(baseMipLevel + level, baseArrayLayer + layer)] = newLayout;
            }
        }
    }
}

void Texture::SetImageLayout(uint32_t mipLevel, uint32_t arrayLayer, VkImageLayout layout)
{
    _imageLayouts[CalculateSubresource(mipLevel, arrayLayer)] = layout;
}

VD_EXPORT VdResult VdTexture_GetDescription(Texture* texture, TextureDescription* description)
{
    return texture->GetDescription(description);
}

VD_EXPORT VdResult VdTexture_Dispose(Texture* texture)
{
    delete texture;
    return VdResult::Success;
}
}
