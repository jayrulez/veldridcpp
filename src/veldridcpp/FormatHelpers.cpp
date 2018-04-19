#include "stdafx.h"
#include "FormatHelpers.hpp"

namespace Veldrid
{
uint32_t GetSizeInBytes(PixelFormat format)
{
    switch (format)
    {
    case PixelFormat::R8_UNorm:
    case PixelFormat::R8_SNorm:
    case PixelFormat::R8_UInt:
    case PixelFormat::R8_SInt:
        return 1;

    case PixelFormat::R16_UNorm:
    case PixelFormat::R16_SNorm:
    case PixelFormat::R16_UInt:
    case PixelFormat::R16_SInt:
    case PixelFormat::R16_Float:
    case PixelFormat::R8_G8_UNorm:
    case PixelFormat::R8_G8_SNorm:
    case PixelFormat::R8_G8_UInt:
    case PixelFormat::R8_G8_SInt:
        return 2;

    case PixelFormat::R32_UInt:
    case PixelFormat::R32_SInt:
    case PixelFormat::R32_Float:
    case PixelFormat::R16_G16_UNorm:
    case PixelFormat::R16_G16_SNorm:
    case PixelFormat::R16_G16_UInt:
    case PixelFormat::R16_G16_SInt:
    case PixelFormat::R16_G16_Float:
    case PixelFormat::R8_G8_B8_A8_UNorm:
    case PixelFormat::R8_G8_B8_A8_SNorm:
    case PixelFormat::R8_G8_B8_A8_UInt:
    case PixelFormat::R8_G8_B8_A8_SInt:
    case PixelFormat::B8_G8_R8_A8_UNorm:
    case PixelFormat::R10_G10_B10_A2_UNorm:
    case PixelFormat::R10_G10_B10_A2_UInt:
    case PixelFormat::R11_G11_B10_Float:
    case PixelFormat::D24_UNorm_S8_UInt:
        return 4;

    case PixelFormat::D32_Float_S8_UInt:
        return 5;

    case PixelFormat::R16_G16_B16_A16_UNorm:
    case PixelFormat::R16_G16_B16_A16_SNorm:
    case PixelFormat::R16_G16_B16_A16_UInt:
    case PixelFormat::R16_G16_B16_A16_SInt:
    case PixelFormat::R16_G16_B16_A16_Float:
    case PixelFormat::R32_G32_UInt:
    case PixelFormat::R32_G32_SInt:
    case PixelFormat::R32_G32_Float:
        return 8;

    case PixelFormat::R32_G32_B32_A32_Float:
    case PixelFormat::R32_G32_B32_A32_UInt:
    case PixelFormat::R32_G32_B32_A32_SInt:
        return 16;

    case PixelFormat::BC1_Rgb_UNorm:
    case PixelFormat::BC1_Rgba_UNorm:
    case PixelFormat::BC2_UNorm:
    case PixelFormat::BC3_UNorm:
    case PixelFormat::ETC2_R8_G8_B8_UNorm:
    case PixelFormat::ETC2_R8_G8_B8_A1_UNorm:
    case PixelFormat::ETC2_R8_G8_B8_A8_UNorm:
        VdFail("GetSizeInBytes should not be used on a compressed format.");

    default:
        VdFail("Invalid PixelFormat.");
    }
}
uint32_t GetSampleCountUInt32(TextureSampleCount sampleCount)
{
    switch (sampleCount)
    {
    case TextureSampleCount::Count1:
        return 1;
    case TextureSampleCount::Count2:
        return 2;
    case TextureSampleCount::Count4:
        return 4;
    case TextureSampleCount::Count8:
        return 8;
    case TextureSampleCount::Count16:
        return 16;
    case TextureSampleCount::Count32:
        return 32;
    default:
        VdFail("Invalid TextureSampleCount value.");
    }
}
bool IsStencilFormat(PixelFormat format)
{
    return format == PixelFormat::D24_UNorm_S8_UInt
        || format == PixelFormat::D32_Float_S8_UInt;
}
bool IsCompressedFormat(PixelFormat format)
{
    return format == PixelFormat::BC1_Rgb_UNorm
        || format == PixelFormat::BC1_Rgba_UNorm
        || format == PixelFormat::BC2_UNorm
        || format == PixelFormat::BC3_UNorm
        || format == PixelFormat::ETC2_R8_G8_B8_UNorm
        || format == PixelFormat::ETC2_R8_G8_B8_A1_UNorm
        || format == PixelFormat::ETC2_R8_G8_B8_A8_UNorm;
}
uint32_t GetBlockSizeInBytes(PixelFormat format)
{
    switch (format)
    {
    case PixelFormat::BC1_Rgba_UNorm:
    case PixelFormat::BC1_Rgb_UNorm:
    case PixelFormat::ETC2_R8_G8_B8_UNorm:
    case PixelFormat::ETC2_R8_G8_B8_A1_UNorm:
        return 8;
    case PixelFormat::BC2_UNorm:
    case PixelFormat::BC3_UNorm:
    case PixelFormat::ETC2_R8_G8_B8_A8_UNorm:
        return 16;
    default:
        VdFail("Invalid PixelFormat value.");
    }
}
uint32_t GetRowPitch(uint32_t width, PixelFormat format)
{
    switch (format)
    {
    case PixelFormat::BC1_Rgba_UNorm:
    case PixelFormat::BC1_Rgb_UNorm:
    case PixelFormat::BC2_UNorm:
    case PixelFormat::BC3_UNorm:
    case PixelFormat::ETC2_R8_G8_B8_UNorm:
    case PixelFormat::ETC2_R8_G8_B8_A1_UNorm:
    case PixelFormat::ETC2_R8_G8_B8_A8_UNorm:
    {
        uint32_t blocksPerRow = (width + 3) / 4;
        uint32_t blockSizeInBytes = GetBlockSizeInBytes(format);
        return blocksPerRow * blockSizeInBytes;
    }

    default:
        return width * GetSizeInBytes(format);
    }
}

uint32_t GetNumRows(uint32_t height, PixelFormat format)
{
    switch (format)
    {
    case PixelFormat::BC1_Rgba_UNorm:
    case PixelFormat::BC1_Rgb_UNorm:
    case PixelFormat::BC2_UNorm:
    case PixelFormat::BC3_UNorm:
    case PixelFormat::ETC2_R8_G8_B8_UNorm:
    case PixelFormat::ETC2_R8_G8_B8_A1_UNorm:
    case PixelFormat::ETC2_R8_G8_B8_A8_UNorm:
        return (height + 3) / 4;

    default:
        return height;
    }
}

uint32_t GetDepthPitch(uint32_t rowPitch, uint32_t height, PixelFormat format)
{
    return rowPitch * GetNumRows(height, format);
}

uint32_t GetRegionSize(uint32_t width, uint32_t height, uint32_t depth, PixelFormat format)
{
    uint32_t blockSizeInBytes;
    if (IsCompressedFormat(format))
    {
        VdAssert((width % 4 == 0 || width < 4) && (height % 4 == 0 || height < 4), "Compressed texture region must be aligned.");
        blockSizeInBytes = GetBlockSizeInBytes(format);
        width /= 4;
        height /= 4;
    }
    else
    {
        blockSizeInBytes = GetSizeInBytes(format);
    }

    return width * height * depth * blockSizeInBytes;
}
uint32_t GetSizeInBytes(VertexElementFormat format)
{
    switch (format)
    {
    case VertexElementFormat::Byte2_Norm:
    case VertexElementFormat::Byte2:
    case VertexElementFormat::SByte2_Norm:
    case VertexElementFormat::SByte2:
        return 2;
    case VertexElementFormat::Float1:
    case VertexElementFormat::UInt1:
    case VertexElementFormat::Int1:
    case VertexElementFormat::Byte4_Norm:
    case VertexElementFormat::Byte4:
    case VertexElementFormat::SByte4_Norm:
    case VertexElementFormat::SByte4:
    case VertexElementFormat::UShort2_Norm:
    case VertexElementFormat::UShort2:
    case VertexElementFormat::Short2_Norm:
    case VertexElementFormat::Short2:
        return 4;
    case VertexElementFormat::Float2:
    case VertexElementFormat::UInt2:
    case VertexElementFormat::Int2:
    case VertexElementFormat::UShort4_Norm:
    case VertexElementFormat::UShort4:
    case VertexElementFormat::Short4_Norm:
    case VertexElementFormat::Short4:
        return 8;
    case VertexElementFormat::Float3:
    case VertexElementFormat::UInt3:
    case VertexElementFormat::Int3:
        return 12;
    case VertexElementFormat::Float4:
    case VertexElementFormat::UInt4:
    case VertexElementFormat::Int4:
        return 16;
    default:
        VdFail("Illegal VertexElementFormat value.");
    }
}
}
