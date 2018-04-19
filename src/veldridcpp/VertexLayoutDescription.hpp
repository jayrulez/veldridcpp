#pragma once
#include "stdint.h"
#include "InteropArray.hpp"
#include "VertexElementDescription.hpp"

namespace Veldrid
{
struct VertexLayoutDescription
{
    uint32_t Stride;
    InteropArray<VertexElementDescription> Elements;
    uint32_t InstanceStepRate;
};
}
