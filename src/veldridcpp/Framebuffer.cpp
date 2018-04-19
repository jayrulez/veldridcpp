#include "stdafx.h"
#include "FormatHelpers.hpp"
#include "Framebuffer.hpp"
#include "Util.hpp"
#include "VeldridConfig.hpp"
#include "stdint.h"
#include <optional>

namespace Veldrid
{
Framebuffer::Framebuffer(GraphicsDevice* gd, const FramebufferDescription& description, bool isPresented)
{
    _gd = gd;
    _colorTargets.resize(description.ColorTargets.Count);
    for (uint32_t i = 0; i < ColorTargets().size(); i++)
    {
        _colorTargets[i] = description.ColorTargets[i];
    }

    if (description.DepthTarget != nullptr)
    {
        _depthTarget = *description.DepthTarget;
    }

    VkRenderPassCreateInfo renderPassCI = {};
    renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

    std::vector<VkAttachmentDescription> attachments;

    _colorAttachmentCount = description.ColorTargets.Count;
    std::vector<VkAttachmentReference> colorAttachmentRefs;
    for (uint32_t i = 0; i < _colorAttachmentCount; i++)
    {
        Texture* vkColorTex = description.ColorTargets[i].Target;
        VkAttachmentDescription colorAttachmentDesc = {};
        colorAttachmentDesc.format = vkColorTex->GetVkFormat();
        colorAttachmentDesc.samples = vkColorTex->GetVkSampleCount();
        colorAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachments.push_back(colorAttachmentDesc);

        VkAttachmentReference colorAttachmentRef;
        colorAttachmentRef.attachment = (uint32_t)i;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachmentRefs.push_back(colorAttachmentRef);
    }

    VkAttachmentDescription depthAttachmentDesc;
    VkAttachmentReference depthAttachmentRef;
    if (description.DepthTarget != nullptr)
    {
        Texture* vkDepthTex = description.DepthTarget->Target;
        bool hasStencil = IsStencilFormat(vkDepthTex->GetFormat());
        depthAttachmentDesc.format = vkDepthTex->GetVkFormat();
        depthAttachmentDesc.samples = vkDepthTex->GetVkSampleCount();
        depthAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachmentDesc.stencilStoreOp = hasStencil ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        depthAttachmentRef.attachment = description.ColorTargets.Count;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    if (description.ColorTargets.Count > 0)
    {
        subpass.colorAttachmentCount = _colorAttachmentCount;
        subpass.pColorAttachments = colorAttachmentRefs.data();
    }

    if (description.DepthTarget != nullptr)
    {
        subpass.pDepthStencilAttachment = &depthAttachmentRef;
        attachments.push_back(depthAttachmentDesc);
    }

    VkSubpassDependency subpassDependency = {};
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    if (description.DepthTarget != nullptr)
    {
        subpassDependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }

    renderPassCI.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassCI.pAttachments = attachments.data();
    renderPassCI.subpassCount = 1;
    renderPassCI.pSubpasses = &subpass;
    renderPassCI.dependencyCount = 1;
    renderPassCI.pDependencies = &subpassDependency;

    VkResult creationResult = vkCreateRenderPass(_gd->GetVkDevice(), &renderPassCI, nullptr, &_renderPassNoClear);
    CheckResult(creationResult);

    for (uint32_t i = 0; i < _colorAttachmentCount; i++)
    {
        attachments[i].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachments[i].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    if (description.DepthTarget != nullptr)
    {
        attachments[attachments.size() - 1].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachments[attachments.size() - 1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        bool hasStencil = IsStencilFormat(description.DepthTarget->Target->GetFormat());
        if (hasStencil)
        {
            attachments[attachments.size() - 1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        }

    }
    creationResult = vkCreateRenderPass(_gd->GetVkDevice(), &renderPassCI, nullptr, &_renderPassNoClearLoad);
    CheckResult(creationResult);


    // Load version

    if (description.DepthTarget != nullptr)
    {
        attachments[attachments.size() - 1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[attachments.size() - 1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        bool hasStencil = IsStencilFormat(description.DepthTarget->Target->GetFormat());
        if (hasStencil)
        {
            attachments[attachments.size() - 1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        }
    }

    for (uint32_t i = 0; i < _colorAttachmentCount; i++)
    {
        attachments[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    }

    creationResult = vkCreateRenderPass(_gd->GetVkDevice(), &renderPassCI, nullptr, &_renderPassClear);
    CheckResult(creationResult);

    VkFramebufferCreateInfo fbCI = {};
    fbCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    uint32_t fbAttachmentsCount = description.ColorTargets.Count;
    if (description.DepthTarget != nullptr)
    {
        fbAttachmentsCount += 1;
    }

    std::vector<VkImageView> fbAttachments(fbAttachmentsCount);
    for (uint32_t i = 0; i < _colorAttachmentCount; i++)
    {
        Texture* vkColorTarget = description.ColorTargets[i].Target;
        VkImageViewCreateInfo imageViewCI = {};
        imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCI.image = vkColorTarget->GetOptimalImage();
        imageViewCI.format = vkColorTarget->GetVkFormat();
        imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCI.subresourceRange.baseMipLevel = description.ColorTargets[i].MipLevel;
        imageViewCI.subresourceRange.levelCount = 1;
        imageViewCI.subresourceRange.baseArrayLayer = description.ColorTargets[i].ArrayLayer;
        imageViewCI.subresourceRange.layerCount = 1;
        VkResult result = vkCreateImageView(_gd->GetVkDevice(), &imageViewCI, nullptr, &fbAttachments[i]);
        CheckResult(result);
        _attachmentViews.push_back(fbAttachments[i]);
    }

    // Depth
    if (description.DepthTarget != nullptr)
    {
        Texture* vkDepthTarget = description.DepthTarget->Target;
        bool hasStencil = IsStencilFormat(vkDepthTarget->GetFormat());
        VkImageViewCreateInfo depthViewCI = {};
        depthViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        depthViewCI.image = vkDepthTarget->GetOptimalImage();
        depthViewCI.format = vkDepthTarget->GetVkFormat();
        depthViewCI.viewType = description.DepthTarget->Target->GetArrayLayers() == 1 ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        depthViewCI.subresourceRange.aspectMask = hasStencil ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT : VK_IMAGE_ASPECT_DEPTH_BIT;
        depthViewCI.subresourceRange.baseMipLevel = description.DepthTarget->MipLevel;
        depthViewCI.subresourceRange.levelCount = 1;
        depthViewCI.subresourceRange.baseArrayLayer = description.DepthTarget->ArrayLayer;
        depthViewCI.subresourceRange.layerCount = 1;
        VkResult result = vkCreateImageView(_gd->GetVkDevice(), &depthViewCI, nullptr, &fbAttachments[fbAttachmentsCount - 1]);
        CheckResult(result);
        _attachmentViews.push_back(fbAttachments[fbAttachmentsCount - 1]);
    }

    Texture* dimTex;
    uint32_t mipLevel;
    if (description.ColorTargets.Count > 0)
    {
        dimTex = description.ColorTargets[0].Target;
        mipLevel = description.ColorTargets[0].MipLevel;
    }
    else
    {
        VdAssert(description.DepthTarget != nullptr);
        dimTex = description.DepthTarget->Target;
        mipLevel = description.DepthTarget->MipLevel;
    }

    uint32_t mipWidth, mipHeight, mipDepth;
    GetMipDimensions(dimTex, mipLevel, &mipWidth, &mipHeight, &mipDepth);

    _renderableWidth = mipWidth;
    _renderableHeight = mipHeight;

    fbCI.width = mipWidth;
    fbCI.height = mipHeight;

    fbCI.attachmentCount = fbAttachmentsCount;
    fbCI.pAttachments = fbAttachments.data();
    fbCI.layers = 1;
    fbCI.renderPass = _renderPassNoClear;

    creationResult = vkCreateFramebuffer(_gd->GetVkDevice(), &fbCI, nullptr, &_fb);
    CheckResult(creationResult);

    if (description.DepthTarget != nullptr)
    {
        _attachmentCount += 1;
    }
    _attachmentCount += description.ColorTargets.Count;
}

Framebuffer::~Framebuffer()
{
    vkDestroyFramebuffer(_gd->GetVkDevice(), _fb, nullptr);
    vkDestroyRenderPass(_gd->GetVkDevice(), _renderPassNoClear, nullptr);
    vkDestroyRenderPass(_gd->GetVkDevice(), _renderPassNoClearLoad, nullptr);
    vkDestroyRenderPass(_gd->GetVkDevice(), _renderPassClear, nullptr);
    for (auto& view : _attachmentViews)
    {
        vkDestroyImageView(_gd->GetVkDevice(), view, nullptr);
    }
}

void Framebuffer::TransitionToFinalLayout(VkCommandBuffer cb)
{
}

VD_EXPORT VdResult VdFramebuffer_Dispose(Framebuffer* fb)
{
    delete fb;
    return VdResult::Success;
}
}
