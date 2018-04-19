#pragma once
#include "InteropArray.hpp"
#include "ShaderStages.hpp"
#include "stdint.h"

namespace Veldrid
{
struct ShaderDescription
{
    ShaderStages Stage;
    InteropArray<uint8_t> ShaderBytes;
};
}