#pragma once
#include <stdint.h>


namespace Veldrid
{
typedef void(*ErrorCallback) (uint32_t errorCode, const char* errorMessage);

struct GraphicsDeviceCallbacks
{
    ErrorCallback OnError;
};
}
