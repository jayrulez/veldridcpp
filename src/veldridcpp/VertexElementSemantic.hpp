#pragma once
#include "stdint.h"

namespace Veldrid
{
enum class VertexElementSemantic : uint8_t
{
    Position,
    Normal,
    TextureCoordinate,
    Color,
};
}