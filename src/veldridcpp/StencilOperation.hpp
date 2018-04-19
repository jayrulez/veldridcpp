#pragma once
#include "stdint.h"

namespace Veldrid
{
enum class StencilOperation : uint8_t
{
    Keep,
    Zero,
    Replace,
    IncrementAndClamp,
    DecrementAndClamp,
    Invert,
    IncrementAndWrap,
    DecrementAndWrap,
};
}