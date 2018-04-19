#pragma once
#include "BlendStateDescription.hpp"
#include "DepthStencilStateDescription.hpp"
#include "RasterizerStateDescription.hpp"
#include "PrimitiveTopology.hpp"
#include "ShaderSetDescription.hpp"
#include "InteropArray.hpp"
#include "ResourceLayout.hpp"
#include "InteropOutputDescription.hpp"
#include "ResourceBindingModel.hpp"

namespace Veldrid
{
struct GraphicsPipelineDescription
{
    BlendStateDescription BlendState;
    DepthStencilStateDescription DepthStencilState;
    RasterizerStateDescription RasterizerState;
    PrimitiveTopology PrimitiveTopology;
    ShaderSetDescription ShaderSet;
    InteropArray<ResourceLayout*> ResourceLayouts;
    InteropOutputDescription Outputs;
    ResourceBindingModel ResourceBindingModel;
};
}