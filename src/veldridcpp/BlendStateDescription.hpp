#pragma once
#include "RgbaFloat.hpp"
#include "InteropArray.hpp"
#include "BlendAttachmentDescription.hpp"

namespace Veldrid
{
struct BlendStateDescription
{
    RgbaFloat BlendFactor;
    InteropArray<BlendAttachmentDescription> AttachmentStates;
};
}