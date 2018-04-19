#pragma once
#include "VertexElementSemantic.hpp"
#include "VertexElementFormat.hpp"

namespace Veldrid
{
struct VertexElementDescription
{
    VertexElementSemantic Semantic;
    VertexElementFormat Format;
};
}