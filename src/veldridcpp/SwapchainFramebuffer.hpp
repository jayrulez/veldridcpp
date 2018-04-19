#pragma once
#include "Framebuffer.hpp"
#include "FramebufferBase.hpp"
#include "GraphicsDevice.hpp"
#include "PixelFormat.hpp"
#include "OutputDescription.hpp"
#include "stdint.h"
#include <optional>
#include <vector>

namespace Veldrid
{
class SwapchainFramebuffer : public FramebufferBase
{
public:
    SwapchainFramebuffer(
        GraphicsDevice* gd,
        VkSurfaceKHR surface,
        uint32_t width,
        uint32_t height,
        PixelFormat* depthFormat);

    VkFramebuffer GetCurrentFramebuffer() const override { return _scFramebuffers[_currentImageIndex]->GetCurrentFramebuffer(); }
    VkRenderPass GetRenderPassNoClear_Init() const override { return _scFramebuffers[0]->GetRenderPassNoClear_Init(); }
    VkRenderPass GetRenderPassNoClear_Load() const override { return _scFramebuffers[0]->GetRenderPassNoClear_Load(); }
    VkRenderPass GetRenderPassClear() const override { return _scFramebuffers[0]->GetRenderPassClear(); }
    uint32_t GetAttachmentCount() const override { return _attachmentCount; }
    uint32_t GetColorAttachmentCount() const override { return 1u; }
    const std::vector<FramebufferAttachmentDescription>& ColorTargets() const override { return _scColorTextures[_currentImageIndex]; }
    const std::optional<FramebufferAttachmentDescription>& DepthTarget() const override { return _depthTarget; }
    uint32_t Width() const override { return _desiredWidth; }
    uint32_t Height() const override { return _desiredHeight; }
    uint32_t RenderableWidth() const override { return _scExtent.width; }
    uint32_t RenderableHeight() const override { return _scExtent.height; }
    void TransitionToFinalLayout(VkCommandBuffer cb) override;

    void SetImageIndex(uint32_t index) { _currentImageIndex = index; }
    void SetNewSwapchain(
        VkSwapchainKHR deviceSwapchain,
        uint32_t width,
        uint32_t height,
        VkSurfaceFormatKHR surfaceFormat,
        VkExtent2D swapchainExtent);
    void CreateDepthTexture();
    void CreateFramebuffers();

private:
    GraphicsDevice * _gd;
    VkSurfaceKHR _surface;
    uint32_t _attachmentCount;
    std::optional<PixelFormat> _depthFormat;
    std::optional<FramebufferAttachmentDescription> _depthTarget;

    uint32_t _currentImageIndex;
    std::vector<Framebuffer*> _scFramebuffers;
    std::vector<VkImage> _scImages;
    VkFormat _scImageFormat;
    VkExtent2D _scExtent;
    std::vector<std::vector<FramebufferAttachmentDescription>> _scColorTextures;
    
    uint32_t _desiredWidth;
    uint32_t _desiredHeight;
    OutputDescription _outputDescription;
};
}