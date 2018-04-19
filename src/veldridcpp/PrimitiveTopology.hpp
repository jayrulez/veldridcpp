#pragma once
#include "stdint.h"

namespace Veldrid
{
enum class PrimitiveTopology : uint8_t
{
    TriangleList,
    TriangleStrip,
    LineList,
    LineStrip,
    PointList,
};
}