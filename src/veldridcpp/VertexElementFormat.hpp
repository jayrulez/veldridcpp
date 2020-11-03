#pragma once
#include "stdint.h"

namespace Veldrid
{
enum class VertexElementFormat : uint8_t
{
    Float1,
    Float2,
    Float3,
    Float4,
    Byte2_Norm,
    Byte2,
    Byte4_Norm,
    Byte4,
    SByte2_Norm,
    SByte2,
    SByte4_Norm,
    SByte4,
    UShort2_Norm,
    UShort2,
    UShort4_Norm,
    UShort4,
    Short2_Norm,
    Short2,
    Short4_Norm,
    Short4,
    UInt1,
    UInt2,
    UInt3,
    UInt4,
    Int1,
    Int2,
    Int3,
    Int4,
};
}