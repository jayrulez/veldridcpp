#pragma once
#include "InteropArray.hpp"
#include "VertexLayoutDescription.hpp"
#include "Shader.hpp"

namespace Veldrid
{
struct ShaderSetDescription
{
    InteropArray<VertexLayoutDescription> VertexLayouts;
    InteropArray<Shader*> Shaders;
};
}
