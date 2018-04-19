#pragma once
#include "FaceCullMode.hpp"
#include "PolygonFillMode.hpp"
#include "FrontFace.hpp"

namespace Veldrid
{
struct RasterizerStateDescription
{
    FaceCullMode CullMode;
    PolygonFillMode FillMode;
    FrontFace FrontFace;
    bool DepthClipEnabled;
    bool ScissorTestEnabled;
};
}
