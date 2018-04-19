#pragma once
#include "Optional.hpp"
#include "FramebufferAttachmentDescription.hpp"
#include "stdint.h"
#include "vulkan.h"
#include <vector>
#include <optional>

namespace Veldrid
{
class FramebufferBase
{
public:
    virtual VkFramebuffer GetCurrentFramebuffer() const = 0;
    virtual VkRenderPass GetRenderPassNoClear_Init() const = 0;
    virtual VkRenderPass GetRenderPassNoClear_Load() const = 0;
    virtual VkRenderPass GetRenderPassClear() const = 0;
    virtual uint32_t GetAttachmentCount() const = 0;
    virtual uint32_t GetColorAttachmentCount() const = 0;
    virtual const std::vector<FramebufferAttachmentDescription>& ColorTargets() const = 0;
    virtual const std::optional<FramebufferAttachmentDescription>& DepthTarget() const = 0;
    virtual uint32_t Width() const = 0;
    virtual uint32_t Height() const = 0;
    virtual uint32_t RenderableWidth() const = 0;
    virtual uint32_t RenderableHeight() const = 0;

    virtual void TransitionToFinalLayout(VkCommandBuffer cb) = 0;

    VdResult GetColorTargets(uint32_t* count, FramebufferAttachmentDescription* descriptions);
    VdResult GetDepthTarget(FramebufferAttachmentDescription* description);
};
}
