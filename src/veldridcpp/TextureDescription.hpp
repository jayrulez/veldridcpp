#pragma once
#include "stdint.h"
#include "PixelFormat.hpp"
#include "TextureUsage.hpp"
#include "TextureType.hpp"
#include "TextureSampleCount.hpp"

namespace Veldrid
{
struct TextureDescription
{
    uint32_t Width;
    uint32_t Height;
    uint32_t Depth;
    uint32_t MipLevels;
    uint32_t ArrayLayers;
    PixelFormat Format;
    TextureUsage Usage;
    TextureType Type;
    TextureSampleCount SampleCount;

    static TextureDescription Texture2D(
        uint32_t width,
        uint32_t height,
        uint32_t mipLevels,
        uint32_t arrayLayers,
        PixelFormat format,
        TextureUsage usage)
    {
        TextureDescription ret;
        ret.Type = TextureType::Texture2D;
        ret.Width = width;
        ret.Height = height;
        ret.Depth = 1;
        ret.MipLevels = mipLevels;
        ret.ArrayLayers = arrayLayers;
        ret.Format = format;
        ret.Usage = usage;
        ret.SampleCount = TextureSampleCount::Count1;
        return ret;
    }

    static TextureDescription Texture3D(
        uint32_t width,
        uint32_t height,
        uint32_t depth,
        uint32_t mipLevels,
        PixelFormat format,
        TextureUsage usage)
    {
        TextureDescription ret;
        ret.Type = TextureType::Texture3D;
        ret.Width = width;
        ret.Height = height;
        ret.Depth = depth;
        ret.MipLevels = mipLevels;
        ret.ArrayLayers = 1;
        ret.Format = format;
        ret.Usage = usage;
        ret.SampleCount = TextureSampleCount::Count1;
        return ret;
    }
};
}