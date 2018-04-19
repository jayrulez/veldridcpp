#pragma once
#include "BlendFactor.hpp"
#include "BlendFunction.hpp"

namespace Veldrid
{
struct BlendAttachmentDescription
{
    bool BlendEnabled;
    BlendFactor SourceColorFactor;
    BlendFactor DestinationColorFactor;
    BlendFunction ColorFunction;
    BlendFactor SourceAlphaFactor;
    BlendFactor DestinationAlphaFactor;
    BlendFunction AlphaFunction;
};
}