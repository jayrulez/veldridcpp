#pragma once
#include "PixelFormat.hpp"
#include "TextureSampleCount.hpp"
#include "TextureType.hpp"
#include "TextureUsage.hpp"
#include "VeldridConfig.hpp"
#include "ResourceKind.hpp"
#include "ShaderStages.hpp"
#include "BlendFactor.hpp"
#include "BlendFunction.hpp"
#include "FaceCullMode.hpp"
#include "PolygonFillMode.hpp"
#include "ComparisonKind.hpp"
#include "StencilOperation.hpp"
#include "PrimitiveTopology.hpp"
#include "VertexElementFormat.hpp"
#include "IndexFormat.hpp"
#include "SamplerFilter.hpp"
#include "SamplerAddressMode.hpp"
#include "SamplerBorderColor.hpp"
#include "vulkan.h"

namespace Veldrid
{
VkSampleCountFlags VdToVkSampleCount(TextureSampleCount sampleCount);
VkFormat VdToVkPixelFormat(PixelFormat format, bool toDepthFormat = false);
PixelFormat VkToVdPixelFormat(VkFormat vkFormat);
VkImageType VdToVkTextureType(TextureType type);
VkImageUsageFlags VdToVkTextureUsage(TextureUsage vdUsage);
VkDescriptorType VdToVkDescriptorType(ResourceKind kind);
VkShaderStageFlags VdToVkShaderStages(ShaderStages stage);
VkBlendFactor VdToVkBlendFactor(BlendFactor factor);
VkBlendOp VdToVkBlendOp(BlendFunction func);
VkCullModeFlags VdToVkCullMode(FaceCullMode cullMode);
VkPolygonMode VdToVkPolygonMode(PolygonFillMode fillMode);
VkCompareOp VdToVkCompareOp(ComparisonKind comparisonKind);
VkStencilOp VdToVkStencilOp(StencilOperation op);
VkPrimitiveTopology VdToVkPrimitiveTopology(PrimitiveTopology topology);
VkFormat VdToVkVertexElementFormat(VertexElementFormat format);
VkIndexType VdToVkIndexFormat(IndexFormat format);
void GetFilterParams(SamplerFilter filter, VkFilter* minFilter, VkFilter* magFilter, VkSamplerMipmapMode* mipmapMode);
VkSamplerAddressMode VdToVkSamplerAddressMode(SamplerAddressMode mode);
VkBorderColor VdToVkSamplerBorderColor(SamplerBorderColor borderColor);
}