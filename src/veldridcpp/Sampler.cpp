#include "stdafx.h"
#include "Sampler.hpp"
#include "VeldridConfig.hpp"
#include "VkFormats.hpp"

namespace Veldrid
{
Sampler::Sampler(GraphicsDevice* gd, const SamplerDescription& description)
{
    _gd = gd;

    VkFilter minFilter, magFilter;
    VkSamplerMipmapMode mipmapMode;
    GetFilterParams(description.Filter, &minFilter, &magFilter, &mipmapMode);

    VkSamplerCreateInfo samplerCI = {};
    samplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCI.addressModeU = VdToVkSamplerAddressMode(description.AddressModeU);
    samplerCI.addressModeV = VdToVkSamplerAddressMode(description.AddressModeV);
    samplerCI.addressModeW = VdToVkSamplerAddressMode(description.AddressModeW);
    samplerCI.minFilter = minFilter;
    samplerCI.magFilter = magFilter;
    samplerCI.mipmapMode = mipmapMode;
    samplerCI.compareEnable = description.ComparisonKind != nullptr;
    samplerCI.compareOp = description.ComparisonKind != nullptr
        ? VdToVkCompareOp(*description.ComparisonKind)
        : VkCompareOp::VK_COMPARE_OP_NEVER;
    samplerCI.anisotropyEnable = description.Filter == SamplerFilter::Anisotropic;
    samplerCI.maxAnisotropy = static_cast<float>(description.MaximumAnisotropy);
    samplerCI.minLod = static_cast<float>(description.MinimumLod);
    samplerCI.maxLod = static_cast<float>(description.MaximumLod);
    samplerCI.mipLodBias = static_cast<float>(description.LodBias);
    samplerCI.borderColor = VdToVkSamplerBorderColor(description.BorderColor);

    CheckResult(vkCreateSampler(_gd->GetVkDevice(), &samplerCI, nullptr, &_vkSampler));
}

Sampler::~Sampler()
{
    vkDestroySampler(_gd->GetVkDevice(), _vkSampler, nullptr);
}

VD_EXPORT void VdSampler_Dispose(Sampler* sampler) { delete sampler; }
}
