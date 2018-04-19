#pragma once
#include "stdint.h"

namespace Veldrid
{
enum class BlendFactor : uint8_t
{
    Zero,
    One,
    SourceAlpha,
    InverseSourceAlpha,
    DestinationAlpha,
    InverseDestinationAlpha,
    SourceColor,
    InverseSourceColor,
    DestinationColor,
    InverseDestinationColor,
    BlendFactor,
    InverseBlendFactor,
};
}