#pragma once
#include "Framebuffer.hpp"
#include "PixelFormat.hpp"
#include "TextureSampleCount.hpp"
#include <optional>
namespace Veldrid
{
struct OutputDescription
{
    std::optional<PixelFormat> DepthFormat;
    std::vector<PixelFormat> ColorFormats;
    TextureSampleCount SampleCount;

    OutputDescription() {}

    OutputDescription(
        std::optional<PixelFormat> depthFormat,
        std::vector<PixelFormat> colorFormats,
        TextureSampleCount sampleCount)
    {
        DepthFormat = depthFormat;
        ColorFormats = colorFormats;
        SampleCount = sampleCount;
    }

    static OutputDescription CreateFromFramebuffer(const FramebufferBase& fb)
    {
        TextureSampleCount sampleCount = TextureSampleCount::Count1;
        std::optional<PixelFormat> depthFormat = {};
        if (fb.DepthTarget().has_value())
        {
            depthFormat = fb.DepthTarget().value().Target->GetFormat();
            sampleCount = fb.DepthTarget().value().Target->GetSampleCount();
        }

        std::vector<PixelFormat> colorFormats(fb.ColorTargets().size());
        for (int i = 0; i < colorFormats.size(); i++)
        {
            colorFormats[i] = fb.ColorTargets()[i].Target->GetFormat();
            sampleCount = fb.ColorTargets()[i].Target->GetSampleCount();
        }

        return OutputDescription(depthFormat, colorFormats, sampleCount);
    }
};
}