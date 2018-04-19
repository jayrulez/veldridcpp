#pragma once
#include "ComparisonKind.hpp"
#include "StencilBehaviorDescription.hpp"
#include "stdint.h"

namespace Veldrid
{
struct DepthStencilStateDescription
{
    bool DepthTestEnabled;
    bool DepthWriteEnabled;
    ComparisonKind DepthComparison;

    bool StencilTestEnabled;
    StencilBehaviorDescription StencilFront;
    StencilBehaviorDescription StencilBack;
    uint8_t StencilReadMask;
    uint8_t StencilWriteMask;
    uint32_t StencilReference;
};
}