#pragma once
#include "FramebufferAttachmentDescription.hpp"
#include "Texture.hpp"
#include "InteropArray.hpp"
#include "stdint.h"

namespace Veldrid
{
struct FramebufferDescription
{
    FramebufferAttachmentDescription* DepthTarget;
    InteropArray<FramebufferAttachmentDescription> ColorTargets;
};
}