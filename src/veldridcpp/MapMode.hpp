#pragma once
#include "stdint.h"

namespace Veldrid
{
enum class MapMode : uint8_t
{
    Read,
    Write,
    ReadWrite,
};
}