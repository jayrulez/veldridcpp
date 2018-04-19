#pragma once
#include "PixelFormat.hpp"
#include "InteropArray.hpp"
#include "TextureSampleCount.hpp"

namespace Veldrid
{
struct InteropOutputDescription
{
    PixelFormat* DepthAttachment;
    InteropArray<PixelFormat> ColorAttachments;
    TextureSampleCount SampleCount;
};
}
