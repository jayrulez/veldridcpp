#pragma once
#include "stdint.h"

namespace Veldrid
{
enum class ShaderStages : uint8_t
{
    None = 0,
    Vertex = 1 << 0,
    Geometry = 1 << 1,
    TessellationControl = 1 << 2,
    TessellationEvaluation = 1 << 3,
    Fragment = 1 << 4,
    Compute = 1 << 5,
};

inline ShaderStages operator &(const ShaderStages& left, const ShaderStages& right)
{
    return ShaderStages(static_cast<uint8_t>(left) & static_cast<uint8_t>(right));
}
}