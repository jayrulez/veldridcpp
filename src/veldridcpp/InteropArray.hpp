#pragma once
#include "stdint.h"

namespace Veldrid
{
template<class T>
struct InteropArray
{
    uint32_t Count;
    T* Data;

    T& operator[](uint32_t i) const { return Data[i]; }
};
}