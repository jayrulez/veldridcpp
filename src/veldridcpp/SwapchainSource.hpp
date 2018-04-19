#pragma once
#ifdef _WINDOWS
#include "Windows.h"
#endif

namespace Veldrid
{
struct SwapchainSource
{
    HINSTANCE hinstance;
    HWND hwnd;
};
}