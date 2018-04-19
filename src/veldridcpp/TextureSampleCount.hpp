#pragma once
#include "stdint.h"

namespace Veldrid
{
enum class TextureSampleCount : uint8_t
{
    Count1,
    Count2,
    Count4,
    Count8,
    Count16,
    Count32,
};
}