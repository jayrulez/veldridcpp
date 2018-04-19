#pragma once
#include "Texture.hpp"
#include "stdint.h"

namespace Veldrid
{
struct FramebufferAttachmentDescription
{
    Texture* Target;
    uint32_t ArrayLayer;
    uint32_t MipLevel;

    FramebufferAttachmentDescription() { } // TODO: This shouldn't really exist.

    FramebufferAttachmentDescription(Texture* target, uint32_t arrayLayer, uint32_t mipLevel)
    {
        Target = target;
        ArrayLayer = arrayLayer;
        MipLevel = mipLevel;
    }
};
}