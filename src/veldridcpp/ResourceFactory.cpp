#include "stdafx.h"
#include "ResourceFactory.hpp"
#include "VeldridConfig.hpp"
#include "Sampler.hpp"
#include "Framebuffer.hpp"

namespace Veldrid
{
DeviceBuffer* ResourceFactory::CreateBuffer(const BufferDescription& description) const
{
    return new DeviceBuffer(_device, description);
}

Sampler* ResourceFactory::CreateSampler(const SamplerDescription& description) const
{
    return new Sampler(_device, description);
}

Texture* ResourceFactory::CreateTexture(const TextureDescription& description) const
{
    return new Texture(_device, description);
}

CommandList* ResourceFactory::CreateCommandList() const
{
    return new CommandList(_device);
}

Framebuffer* ResourceFactory::CreateFramebuffer(const FramebufferDescription &description) const
{
    return new Framebuffer(_device, description, false);
}

Swapchain* ResourceFactory::CreateSwapchain(const SwapchainDescription & description) const
{
    return new Swapchain(_device, description, VK_NULL_HANDLE);
}

Shader* ResourceFactory::CreateShader(const ShaderDescription& description) const
{
    return new Shader(_device, description);
}

ResourceLayout* ResourceFactory::CreateResourceLayout(const ResourceLayoutDescription & description) const
{
    return new ResourceLayout(_device, description);
}

ResourceSet* ResourceFactory::CreateResourceSet(const ResourceSetDescription & description) const
{
    return new ResourceSet(_device, description);
}

TextureView* ResourceFactory::CreateTextureView(const TextureViewDescription& description) const
{
    return new TextureView(_device, description);
}

Pipeline* ResourceFactory::CreateGraphicsPipeline(const GraphicsPipelineDescription& description) const
{
    return new Pipeline(_device, description);
}

VD_EXPORT DeviceBuffer* VdResourceFactory_CreateBuffer(ResourceFactory* factory, BufferDescription* description)
{
    return factory->CreateBuffer(*description);
}

VD_EXPORT Sampler* VdResourceFactory_CreateSampler(ResourceFactory* factory, SamplerDescription* description)
{
    return factory->CreateSampler(*description);
}

VD_EXPORT CommandList* VdResourceFactory_CreateCommandList(ResourceFactory* factory)
{
    return factory->CreateCommandList();
}

VD_EXPORT Texture* VdResourceFactory_CreateTexture(ResourceFactory* factory, TextureDescription* description)
{
    return factory->CreateTexture(*description);
}

VD_EXPORT Framebuffer* VdResourceFactory_CreateFramebuffer(ResourceFactory* factory, FramebufferDescription* description)
{
    return factory->CreateFramebuffer(*description);
}

VD_EXPORT Swapchain* VdResourceFactory_CreateSwapchain(ResourceFactory* factory, SwapchainDescription* description)
{
    return factory->CreateSwapchain(*description);
}

VD_EXPORT Shader* VdResourceFactory_CreateShader(ResourceFactory* factory, ShaderDescription* description)
{
    return factory->CreateShader(*description);
}

VD_EXPORT ResourceLayout* VdResourceFactory_CreateResourceLayout(ResourceFactory* factory, ResourceLayoutDescription* description)
{
    return factory->CreateResourceLayout(*description);
}

VD_EXPORT TextureView* VdResourceFactory_CreateTextureView(ResourceFactory* factory, TextureViewDescription* description)
{
    return factory->CreateTextureView(*description);
}

VD_EXPORT ResourceSet* VdResourceFactory_CreateResourceSet(ResourceFactory* factory, ResourceSetDescription* description)
{
    return factory->CreateResourceSet(*description);
}

VD_EXPORT Pipeline* VdResourceFactory_CreateGraphicsPipeline(ResourceFactory* factory, GraphicsPipelineDescription* description)
{
    return factory->CreateGraphicsPipeline(*description);
}
}

