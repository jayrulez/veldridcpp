#pragma once
#include "ResourceLayout.hpp"
#include "InteropArray.hpp"

namespace Veldrid
{
struct ResourceSetDescription
{
    ResourceLayout* Layout;
    InteropArray<void*> BoundResources;
};
}