#pragma once
#include "stdint.h"
#include "PixelFormat.hpp"
#include "VeldridConfig.hpp"
#include "TextureSampleCount.hpp"
#include "VertexElementFormat.hpp"
#include <assert.h>

namespace Veldrid
{
uint32_t GetSizeInBytes(PixelFormat format);
uint32_t GetSampleCountUInt32(TextureSampleCount sampleCount);
bool IsStencilFormat(PixelFormat format);
bool IsCompressedFormat(PixelFormat format);
uint32_t GetBlockSizeInBytes(PixelFormat format);
uint32_t GetRowPitch(uint32_t width, PixelFormat format);
uint32_t GetNumRows(uint32_t height, PixelFormat format);
uint32_t GetDepthPitch(uint32_t rowPitch, uint32_t height, PixelFormat format);
uint32_t GetRegionSize(uint32_t width, uint32_t height, uint32_t depth, PixelFormat format);
uint32_t GetSizeInBytes(VertexElementFormat format);
}