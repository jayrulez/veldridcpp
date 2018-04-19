#pragma once
#include "stdint.h"

namespace Veldrid
{
enum class BlendFunction : uint8_t
{
    Add,
    Subtract,
    ReverseSubtract,
    Minimum,
    Maximum,
};
}