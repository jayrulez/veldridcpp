#pragma once

#include "MapMode.hpp"
#include "stdint.h"

namespace Veldrid
{
struct MappedResource
{
    MapMode Mode;
    void* Data;
    uint32_t SizeInBytes;
    uint32_t Subresource;
    uint32_t RowPitch;
    uint32_t DepthPitch;
};
}