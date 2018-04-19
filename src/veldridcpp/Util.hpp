#pragma once

#include "stdint.h"
#include "Texture.hpp"
#include <algorithm>
#include "FormatHelpers.hpp"

namespace Veldrid
{

template<class T>
const inline bool HasFlag(T val, T flag)
{
    return (val & flag) == flag;
}

inline uint32_t GetDimension(uint32_t largestLevelDimension, uint32_t mipLevel)
{
    uint32_t ret = largestLevelDimension;
    for (uint32_t i = 0; i < mipLevel; i++)
    {
        ret /= 2;
    }
    return std::max(1u, ret);
}

inline void GetMipDimensions(
    const Texture* texture,
    uint32_t level,
    uint32_t* mipWidth,
    uint32_t* mipHeight,
    uint32_t* mipDepth)
{
    *mipWidth = GetDimension(texture->GetWidth(), level);
    *mipHeight = GetDimension(texture->GetHeight(), level);
    *mipDepth = GetDimension(texture->GetDepth(), level);
}

inline void GetMipLevelAndArrayLayer(
    const Texture* tex,
    uint32_t subresource,
    uint32_t* mipLevel,
    uint32_t* arrayLayer)
{
    *arrayLayer = subresource / tex->GetMipLevels();
    *mipLevel = subresource - (*arrayLayer * tex->GetMipLevels());
}

inline uint32_t ComputeArrayLayerOffset(const Texture* tex, uint32_t arrayLayer)
{
    if (arrayLayer == 0)
    {
        return 0;
    }

    uint32_t blockSize = IsCompressedFormat(tex->GetFormat()) ? 4u : 1u;
    uint32_t layerPitch = 0;
    for (uint32_t level = 0; level < tex->GetMipLevels(); level++)
    {
        uint32_t mipWidth, mipHeight, mipDepth;
        GetMipDimensions(tex, level, &mipWidth, &mipHeight, &mipDepth);
        uint32_t storageWidth = std::max(mipWidth, blockSize);
        uint32_t storageHeight = std::max(mipHeight, blockSize);
        layerPitch += GetRegionSize(storageWidth, storageHeight, mipDepth, tex->GetFormat());
    }

    return layerPitch * arrayLayer;
}

inline uint32_t ComputeMipOffset(const Texture* tex, uint32_t mipLevel)
{
    uint32_t blockSize = IsCompressedFormat(tex->GetFormat()) ? 4u : 1u;
    uint32_t offset = 0;
    for (uint32_t level = 0; level < mipLevel; level++)
    {
        uint32_t mipWidth, mipHeight, mipDepth;
        GetMipDimensions(tex, level, &mipWidth, &mipHeight, &mipDepth);
        uint32_t storageWidth = std::max(mipWidth, blockSize);
        uint32_t storageHeight = std::max(mipHeight, blockSize);
        offset += GetRegionSize(storageWidth, storageHeight, mipDepth, tex->GetFormat());
    }

    return offset;
}

inline uint64_t ComputeSubresourceOffset(const Texture* tex, uint32_t mipLevel, uint32_t arrayLayer)
{
    return ComputeArrayLayerOffset(tex, arrayLayer) + ComputeMipOffset(tex, mipLevel);
}

inline void CopyTextureRegion(
    void* src,
    uint32_t srcX, uint32_t srcY, uint32_t srcZ,
    uint32_t srcRowPitch,
    uint32_t srcDepthPitch,
    void* dst,
    uint32_t dstX, uint32_t dstY, uint32_t dstZ,
    uint32_t dstRowPitch,
    uint32_t dstDepthPitch,
    uint32_t width,
    uint32_t height,
    uint32_t depth,
    PixelFormat format)
{
    uint32_t blockSize = IsCompressedFormat(format) ? 4u : 1u;
    uint32_t blockSizeInBytes = blockSize > 1 ? GetBlockSizeInBytes(format) : GetSizeInBytes(format);
    uint32_t compressedSrcX = srcX / blockSize;
    uint32_t compressedSrcY = srcY / blockSize;
    uint32_t compressedDstX = dstX / blockSize;
    uint32_t compressedDstY = dstY / blockSize;
    uint32_t numRows = GetNumRows(height, format);
    uint32_t rowSize = width / blockSize * blockSizeInBytes;

    if (srcRowPitch == dstRowPitch && srcDepthPitch == dstDepthPitch)
    {
        uint32_t totalCopySize = depth * srcDepthPitch;
        memcpy(dst, src, totalCopySize);
    }
    else
    {
        for (uint32_t zz = 0; zz < depth; zz++)
            for (uint32_t yy = 0; yy < numRows; yy++)
            {
                uint8_t* rowCopyDst = (uint8_t*)dst
                    + dstDepthPitch * (zz + dstZ)
                    + dstRowPitch * (yy + compressedDstY)
                    + blockSizeInBytes * compressedDstX;

                uint8_t* rowCopySrc = (uint8_t*)src
                    + srcDepthPitch * (zz + srcZ)
                    + srcRowPitch * (yy + compressedSrcY)
                    + blockSizeInBytes * compressedSrcX;

                memcpy(rowCopyDst, rowCopySrc, rowSize);
            }
    }
}

template<class T>
inline void EnsureMinimumSize(std::vector<T>& v, uint32_t minSize)
{
    if (v.size() < minSize)
    {
        v.resize(minSize);
    }
}

template<class T>
inline void ClearVector(std::vector<T>& v)
{
    auto size = v.size();
    v.clear();
    v.resize(size);
}

template<class T>
inline bool BlittableEqual(const T& a, const T& b)
{
    return memcmp(&a, &b, sizeof(T)) == 0;
}
}