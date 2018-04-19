#pragma once
#include "stdint.h"

namespace Veldrid
{
enum class ResourceKind : uint8_t
{
    UniformBuffer,
    StructuredBufferReadOnly,
    StructuredBufferReadWrite,
    TextureReadOnly,
    TextureReadWrite,
    Sampler,
};
}