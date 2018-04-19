#pragma once
#include <stdint.h>

namespace Veldrid
{
enum class BufferUsage : uint8_t
{
    VertexBuffer = 1 << 0,
    IndexBuffer = 1 << 1,
    UniformBuffer = 1 << 2,
    StructuredBufferReadOnly = 1 << 3,
    StructuredBufferReadWrite = 1 << 4,
    IndirectBuffer = 1 << 5,
    Dynamic = 1 << 6,
    Staging = 1 << 7,
};

inline BufferUsage operator &(const BufferUsage& left, const BufferUsage& right)
{
    return BufferUsage(static_cast<uint8_t>(left) & static_cast<uint8_t>(right));
}
}