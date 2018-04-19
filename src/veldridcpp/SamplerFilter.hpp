#pragma once
#include "stdint.h"

namespace Veldrid
{
enum class SamplerFilter : uint8_t
{
    MinPoint_MagPoint_MipPoint,
    MinPoint_MagPoint_MipLinear,
    MinPoint_MagLinear_MipPoint,
    MinPoint_MagLinear_MipLinear,
    MinLinear_MagPoint_MipPoint,
    MinLinear_MagPoint_MipLinear,
    MinLinear_MagLinear_MipPoint,
    MinLinear_MagLinear_MipLinear,
    Anisotropic,
};
}
