#pragma once
#include "FramebufferBase.hpp"
#include "FramebufferDescription.hpp"
#include "GraphicsDevice.hpp"
#include "Optional.hpp"
#include "vulkan.h"
#include <vector>
#include "stdint.h"
#include <optional>

namespace Veldrid
{

class Framebuffer : public FramebufferBase
{
public:
    Framebuffer(GraphicsDevice* gd, const FramebufferDescription& description, bool isPresented);
    ~Framebuffer();
    VkFramebuffer GetCurrentFramebuffer() const override { return _fb; }
    VkRenderPass GetRenderPassNoClear_Init() const override { return _renderPassNoClear; }
    VkRenderPass GetRenderPassNoClear_Load() const override { return _renderPassNoClearLoad; }
    VkRenderPass GetRenderPassClear() const override { return _renderPassClear; }
    uint32_t GetColorAttachmentCount() const { return _colorAttachmentCount; }
    uint32_t GetAttachmentCount() const override { return _attachmentCount; }
    const std::vector<FramebufferAttachmentDescription>& ColorTargets() const override { return _colorTargets; }
    const std::optional<FramebufferAttachmentDescription>& DepthTarget() const override { return _depthTarget; }

    uint32_t Width() const override { return _renderableWidth; }
    uint32_t Height() const override { return _renderableHeight; }
    uint32_t RenderableWidth() const override { return _renderableWidth; }
    uint32_t RenderableHeight() const override { return _renderableHeight; }

    void TransitionToFinalLayout(VkCommandBuffer cb) override;

private:
    GraphicsDevice * _gd;
    VkFramebuffer _fb;
    VkRenderPass _renderPassNoClearLoad;
    VkRenderPass _renderPassNoClear;
    VkRenderPass _renderPassClear;
    std::vector<VkImageView> _attachmentViews;
    uint32_t _attachmentCount;
    uint32_t _colorAttachmentCount;
    std::vector<FramebufferAttachmentDescription> _colorTargets;
    std::optional<FramebufferAttachmentDescription> _depthTarget;
    uint32_t _renderableWidth;
    uint32_t _renderableHeight;
};
}