#pragma once
#include "stdint.h"

namespace Veldrid
{
struct DescriptorResourceCounts
{
    uint32_t UniformBufferCount;
    uint32_t SampledImageCount;
    uint32_t SamplerCount;
    uint32_t StorageBufferCount;
    uint32_t StorageImageCount;
};
}