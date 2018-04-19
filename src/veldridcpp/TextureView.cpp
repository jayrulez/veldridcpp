#include "stdafx.h"
#include "TextureView.hpp"
#include "VeldridConfig.hpp"
#include "TextureUsage.hpp"
#include "Util.hpp"
#include "vulkan.h"

namespace Veldrid
{
TextureView::TextureView(GraphicsDevice* gd, const TextureViewDescription& description)
{
    _gd = gd;
    Texture* target = description.Target;
    VkImageViewCreateInfo imageViewCI = {};
    imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCI.image = target->GetOptimalImage();;
    imageViewCI.format = target->GetVkFormat();

    TextureUsage targetUsage = target->GetUsage();
    VkImageAspectFlags aspectFlags;
    if (HasFlag(targetUsage, TextureUsage::DepthStencil))
    {
        aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    else
    {
        aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    imageViewCI.subresourceRange.aspectMask = aspectFlags;
    imageViewCI.subresourceRange.baseMipLevel = description.BaseMipLevel;
    imageViewCI.subresourceRange.levelCount = description.MipLevels;
    imageViewCI.subresourceRange.baseArrayLayer = description.BaseArrayLayer;
    imageViewCI.subresourceRange.layerCount = description.ArrayLayers;

    if (HasFlag(targetUsage, TextureUsage::Cubemap))
    {
        imageViewCI.viewType = description.ArrayLayers == 1 ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
        imageViewCI.subresourceRange.layerCount *= 6;
    }
    else if (target->GetDepth() == 1)
    {
        imageViewCI.viewType = description.ArrayLayers == 1 ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    }
    else
    {
        imageViewCI.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_3D;
    }

    vkCreateImageView(_gd->GetVkDevice(), &imageViewCI, nullptr, &_imageView);
}

TextureView::~TextureView()
{
    vkDestroyImageView(_gd->GetVkDevice(), _imageView, nullptr);
}

VD_EXPORT void VdTextureView_Dispose(TextureView* view)
{
    delete view;
}
}