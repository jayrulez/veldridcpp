#include "stdafx.h"
#include "Shader.hpp"
#include "VulkanUtil.hpp"
#include "VeldridConfig.hpp"

namespace Veldrid
{
Shader::Shader(GraphicsDevice* gd, const ShaderDescription& description)
{
    _gd = gd;
    _stage = description.Stage;

    VkShaderModuleCreateInfo shaderCI;
    shaderCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderCI.pNext = nullptr;
    shaderCI.flags = 0;
    shaderCI.codeSize = description.ShaderBytes.Count;
    shaderCI.pCode = (uint32_t*)description.ShaderBytes.Data;

    CheckResult(vkCreateShaderModule(_gd->GetVkDevice(), &shaderCI, nullptr, &_shaderModule));
}
Shader::~Shader()
{
    vkDestroyShaderModule(_gd->GetVkDevice(), _shaderModule, nullptr);
}

VD_EXPORT void VdShader_Dispose(Shader* shader)
{
    delete shader;
}
}
