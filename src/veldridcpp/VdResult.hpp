#pragma once
#include "stdint.h"

namespace Veldrid
{
enum class VdResult : int32_t
{
    Success = 0,
    InvalidOperation = 1,
    UnsupportedSystem = 2,
    OutOfMemory = 3,
    SwapchainLost = 4,
};
}