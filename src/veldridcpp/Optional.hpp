#pragma once

namespace Veldrid
{
template<class T>
struct Optional
{
    bool HasValue;
    T Value;
};
}