#pragma once
#include "GraphicsDevice.hpp"
#include "TextureViewDescription.hpp"
#include "vulkan.h"

namespace Veldrid
{
class TextureView
{
public:
    TextureView(GraphicsDevice* gd, const TextureViewDescription& description);
    ~TextureView();
    VkImageView GetVkImageView() const { return _imageView; }

private:
    GraphicsDevice * _gd;
    VkImageView _imageView;
};
}