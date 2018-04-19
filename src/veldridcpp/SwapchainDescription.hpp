#pragma once
#include "SwapchainSource.hpp"
#include "PixelFormat.hpp"
#include "stdint.h"

namespace Veldrid
{
struct SwapchainDescription
{
    SwapchainSource Source;
    uint32_t Width;
    uint32_t Height;
    PixelFormat* DepthFormat;
    bool SyncToVerticalBlank;
};
}