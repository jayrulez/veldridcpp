#include "stdafx.h"
#include "GraphicsDevice.hpp"
#include "ResourceFactory.hpp"
#include "SwapchainFramebuffer.hpp"
#include "OutputDescription.hpp"
#include <algorithm>

namespace Veldrid
{
SwapchainFramebuffer::SwapchainFramebuffer(
    GraphicsDevice* gd,
    VkSurfaceKHR surface,
    uint32_t width,
    uint32_t height,
    PixelFormat* depthFormat)
{
    _gd = gd;
    _surface = surface;
    if (depthFormat != nullptr)
    {
        _depthFormat = *depthFormat;
    }

    _attachmentCount = depthFormat != nullptr ? 2u : 1u;
}

void SwapchainFramebuffer::TransitionToFinalLayout(VkCommandBuffer cb)
{
    for (auto& ca : ColorTargets())
    {
        Texture* vkTex = ca.Target;
        vkTex->SetImageLayout(0, ca.ArrayLayer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        vkTex->TransitionImageLayout(cb, 0, 1, ca.ArrayLayer, 1, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    }
}
void SwapchainFramebuffer::SetNewSwapchain(
    VkSwapchainKHR deviceSwapchain,
    uint32_t width,
    uint32_t height,
    VkSurfaceFormatKHR surfaceFormat,
    VkExtent2D swapchainExtent)
{
    _desiredWidth = width;
    _desiredHeight = height;

    // Get the images
    uint32_t scImageCount = 0;
    VkResult result = vkGetSwapchainImagesKHR(_gd->GetVkDevice(), deviceSwapchain, &scImageCount, nullptr);
    CheckResult(result);
    _scImages.resize(scImageCount);
    result = vkGetSwapchainImagesKHR(_gd->GetVkDevice(), deviceSwapchain, &scImageCount, _scImages.data());
    CheckResult(result);

    _scImageFormat = surfaceFormat.format;
    _scExtent = swapchainExtent;

    CreateDepthTexture();
    CreateFramebuffers();

    _outputDescription = OutputDescription::CreateFromFramebuffer(*this);
}
void SwapchainFramebuffer::CreateDepthTexture()
{
    if (_depthFormat.has_value())
    {
        if (_depthTarget.has_value())
        {
            delete _depthTarget.value().Target;
            _depthTarget.reset();
        }

        Texture* depthTexture = _gd->GetResourceFactory()->CreateTexture(TextureDescription::Texture2D(
            std::max(1u, _scExtent.width), std::max(1u, _scExtent.height),
            1u, 1u,
            _depthFormat.value(),
            TextureUsage::DepthStencil));

        _depthTarget.emplace(depthTexture, 0u, 0u);
    }
}

void SwapchainFramebuffer::CreateFramebuffers()
{
    if (_scFramebuffers.size() > 0)
    {
        for (uint32_t i = 0; i < _scFramebuffers.size(); i++)
        {
            auto& colorTargets = _scFramebuffers[i]->ColorTargets();
            for (uint32_t j = 0; j < colorTargets.size(); j++)
            {
                delete colorTargets[j].Target;
            }
            delete _scFramebuffers[i];

        }

        _scFramebuffers.clear();
    }

    _scFramebuffers.resize(_scImages.size());
    _scColorTextures.resize(_scImages.size());
    for (uint32_t i = 0; i < _scImages.size(); i++)
    {
        Texture* colorTex = new Texture(
            _gd,
            std::max(1u, _scExtent.width),
            std::max(1u, _scExtent.height),
            1,
            1,
            _scImageFormat,
            TextureUsage::RenderTarget,
            TextureSampleCount::Count1,
            _scImages[i]);

        FramebufferDescription desc;
        if (_depthTarget.has_value())
        {
            desc.DepthTarget = &_depthTarget.value();
        }
        else
        {
            desc.DepthTarget = nullptr;
        }
        desc.ColorTargets.Count = 1;
        FramebufferAttachmentDescription colorAttachment(colorTex, 0u, 0u);
        desc.ColorTargets.Data = &colorAttachment;
        Framebuffer* fb = new Framebuffer(_gd, desc, true);
        _scFramebuffers[i] = fb;
        std::vector<FramebufferAttachmentDescription> colorTexturesVec;
        colorTexturesVec.push_back(colorAttachment);
        _scColorTextures[i] = colorTexturesVec;
    }
}
}
