#pragma once
#include "StencilOperation.hpp"
#include "ComparisonKind.hpp"

namespace Veldrid
{
struct StencilBehaviorDescription
{
    StencilOperation Fail;
    StencilOperation Pass;
    StencilOperation DepthFail;
    ComparisonKind Comparison;
};
}