#pragma once
#include "stdint.h"

namespace Veldrid
{
enum class FaceCullMode : uint8_t
{
    Back,
    Front,
    None,
};
}