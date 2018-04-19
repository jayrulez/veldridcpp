#pragma once
#include "BufferUsage.hpp"
#include <stdint.h>

namespace Veldrid
{
struct BufferDescription
{
    BufferDescription(uint32_t size, BufferUsage usage)
    {
        SizeInBytes = size;
        Usage = usage;
    }

    uint32_t SizeInBytes;
    BufferUsage Usage;
};
}