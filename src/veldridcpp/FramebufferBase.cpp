#include "stdafx.h"
#include "FramebufferBase.hpp"
#include "VeldridConfig.hpp"

namespace Veldrid
{
VdResult FramebufferBase::GetColorTargets(uint32_t* count, FramebufferAttachmentDescription* descriptions)
{
    if (descriptions == nullptr)
    {
        *count = static_cast<uint32_t>(ColorTargets().size());
    }
    else
    {
        memcpy(descriptions, ColorTargets().data(), sizeof(FramebufferAttachmentDescription) * ColorTargets().size());
    }

    return VdResult::Success;
}
VdResult FramebufferBase::GetDepthTarget(FramebufferAttachmentDescription* description)
{
    if (DepthTarget().has_value())
    {
        *description = DepthTarget().value();
    }

    return VdResult::Success;
}

VD_EXPORT VdResult VdFramebuffer_GetColorTargets(FramebufferBase* fb, uint32_t* count, FramebufferAttachmentDescription* descriptions)
{
    return fb->GetColorTargets(count, descriptions);
}

VD_EXPORT VdResult VdFramebuffer_GetDepthTarget(FramebufferBase* fb, FramebufferAttachmentDescription* description)
{
    return fb->GetDepthTarget(description);
}
}