#pragma once
#include "stdint.h"

namespace Veldrid
{
enum class SamplerAddressMode : uint8_t
{
    Wrap,
    Mirror,
    Clamp,
    Border,
};
}
