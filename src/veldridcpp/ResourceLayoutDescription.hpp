#pragma once
#include "InteropArray.hpp"
#include "ShaderStages.hpp"
#include "ResourceKind.hpp"

namespace Veldrid
{
struct ResourceLayoutElementDescription
{
    ResourceKind Kind;
    ShaderStages Stages;
};

struct ResourceLayoutDescription
{
    InteropArray<ResourceLayoutElementDescription> Elements;
};
}