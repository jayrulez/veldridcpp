#pragma once
#include "stdint.h"
#include "SamplerAddressMode.hpp"
#include "SamplerFilter.hpp"
#include "SamplerBorderColor.hpp"
#include "ComparisonKind.hpp"

namespace Veldrid
{
struct SamplerDescription
{
    SamplerAddressMode AddressModeU;
    SamplerAddressMode AddressModeV;
    SamplerAddressMode AddressModeW;
    SamplerFilter Filter;
    ComparisonKind* ComparisonKind;
    uint32_t MaximumAnisotropy;
    uint32_t MinimumLod;
    uint32_t MaximumLod;
    int32_t LodBias;
    SamplerBorderColor BorderColor;
};
}