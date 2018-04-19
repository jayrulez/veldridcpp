#pragma once
#include "stdint.h"

namespace Veldrid
{
enum class TextureUsage : uint8_t
{
    Sampled = 1 << 0,
    Storage = 1 << 1,
    RenderTarget = 1 << 2,
    DepthStencil = 1 << 3,
    Cubemap = 1 << 4,
    Staging = 1 << 5,
};

inline TextureUsage operator &(const TextureUsage& left, const TextureUsage& right)
{
    return TextureUsage(static_cast<uint8_t>(left) & static_cast<uint8_t>(right));
}
}