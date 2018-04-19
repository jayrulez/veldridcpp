#pragma once
#include <exception>

#define VD_EXPORT extern "C" __declspec(dllexport)

static inline void VdAssert(bool expression)
{
#ifdef _DEBUG
    if (!expression)
    {
        throw std::exception("An expected invariant was violated.");
    }
#endif
}

static inline void VdAssert(bool expression, const char* message)
{
#ifdef _DEBUG
    if (!expression)
    {
        throw std::exception(message);
    }
#endif
}

[[noreturn]] static inline void VdFail(const char* message) { VdAssert(false, message); }
