#pragma once

namespace Veldrid
{
struct RgbaFloat
{
    float R, G, B, A;
    RgbaFloat(float r, float g, float b, float a)
    {
        R = r;
        G = g;
        B = b;
        A = a;
    }
};
}