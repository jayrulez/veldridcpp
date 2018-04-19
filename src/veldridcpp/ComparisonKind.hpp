#pragma once
#include "stdint.h"

namespace Veldrid
{
enum class ComparisonKind : uint8_t
{
    Never,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always,
};
}