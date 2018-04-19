#pragma once
#include "BufferDescription.hpp"
#include "DeviceBuffer.hpp"
#include "VeldridConfig.hpp"
#include "Sampler.hpp"
#include "CommandList.hpp"
#include "Texture.hpp"
#include "Framebuffer.hpp"
#include "Swapchain.hpp"
#include "Shader.hpp"
#include "ResourceLayout.hpp"
#include "TextureView.hpp"
#include "ResourceSet.hpp"
#include "Pipeline.hpp"

namespace Veldrid
{
class ResourceFactory
{
public:
    ResourceFactory(GraphicsDevice* const device)
        : _device(device) {}
    DeviceBuffer* CreateBuffer(const BufferDescription& description) const;
    Sampler* CreateSampler(const SamplerDescription & description) const;
    Texture* CreateTexture(const TextureDescription& description) const;
    CommandList* CreateCommandList() const;
    Framebuffer* CreateFramebuffer(const FramebufferDescription& description) const;
    Swapchain* CreateSwapchain(const SwapchainDescription& description) const;
    Shader* CreateShader(const ShaderDescription& description) const;
    ResourceLayout* CreateResourceLayout(const ResourceLayoutDescription& description) const;
    ResourceSet* CreateResourceSet(const ResourceSetDescription& description) const;
    TextureView* CreateTextureView(const TextureViewDescription& description) const;
    Pipeline* CreateGraphicsPipeline(const GraphicsPipelineDescription& description) const;

private:
    GraphicsDevice * const _device;
};
}