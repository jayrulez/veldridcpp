#pragma once
#include "vulkan.h"
#include "VeldridConfig.hpp"
#include "GraphicsDevice.hpp"
#include "ShaderDescription.hpp"

namespace Veldrid
{
class Shader
{
public:
    Shader(GraphicsDevice* gd, const ShaderDescription& description);
    ~Shader();
    VkShaderModule ShaderModule() const { return _shaderModule; }
    ShaderStages Stage() const { return _stage; }

private:
    GraphicsDevice * _gd;
    VkShaderModule _shaderModule;
    ShaderStages _stage;
};
}