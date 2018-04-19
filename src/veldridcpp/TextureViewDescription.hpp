#pragma once
#include "Texture.hpp"
#include "stdint.h"

namespace Veldrid
{
struct TextureViewDescription
{
    Texture* Target;
    uint32_t BaseMipLevel;
    uint32_t MipLevels;
    uint32_t BaseArrayLayer;
    uint32_t ArrayLayers;
};
}