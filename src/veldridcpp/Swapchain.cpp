#include "stdafx.h"

#ifdef WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include "VeldridConfig.hpp"
#include "VulkanUtil.hpp"
#include "Swapchain.hpp"
#include "Windows.h"
#include "vulkan.h"
#include <stdio.h>
#include <vector>
#include <unordered_set>
#include <algorithm>

namespace Veldrid
{
void Swapchain::SwapBuffers() const
{
}

VdResult Swapchain::Resize(uint32_t width, uint32_t height)
{
    CreateSwapchain(width, height);
    AcquireNextImage(_gd->GetVkDevice(), VK_NULL_HANDLE, _imageAvailableFence);
    vkWaitForFences(_gd->GetVkDevice(), 1, &_imageAvailableFence, true, UINT64_MAX);
    vkResetFences(_gd->GetVkDevice(), 1, &_imageAvailableFence);
    return VdResult::Success;
}

bool Swapchain::AcquireNextImage(VkDevice device, VkSemaphore semaphore, VkFence fence)
{
    if (_newSyncToVBlank.has_value())
    {
        _syncToVBlank = _newSyncToVBlank.value();
        _newSyncToVBlank.reset();
        CreateSwapchain(_framebuffer->Width(), _framebuffer->Width());
    }

    VkResult result = vkAcquireNextImageKHR(
        device,
        _deviceSwapchain,
        UINT64_MAX,
        semaphore,
        fence,
        &_currentImageIndex);
    _framebuffer->SetImageIndex(_currentImageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        CreateSwapchain(_framebuffer->Width(), _framebuffer->Height());

        return false;
    }
    else if (result != VK_SUCCESS)
    {
        VdFail("Could not acquire next image from the Vulkan swapchain.");
    }

    return true;
}

Swapchain::Swapchain(GraphicsDevice* gd, const SwapchainDescription & description, VkSurfaceKHR existingSurface)
{
    _gd = gd;
    _syncToVBlank = description.SyncToVerticalBlank;

    if (existingSurface == VK_NULL_HANDLE)
    {
        VkWin32SurfaceCreateInfoKHR surfaceCI;
        surfaceCI.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        surfaceCI.hinstance = description.Source.hinstance;
        surfaceCI.hwnd = description.Source.hwnd;
        surfaceCI.flags = 0;
        surfaceCI.pNext = nullptr;
        CheckResult(vkCreateWin32SurfaceKHR(_gd->GetInstance(), &surfaceCI, nullptr, &_surface));
    }
    else
    {
        _surface = existingSurface;
    }

    if (!GetPresentQueueIndex(&_presentQueueIndex))
    {
        VdFail("The system does not support presenting the given Vulkan surface.");
    }
    vkGetDeviceQueue(_gd->GetVkDevice(), _presentQueueIndex, 0, &_presentQueue);

    _framebuffer = new SwapchainFramebuffer(gd, _surface, description.Width, description.Height, description.DepthFormat);

    if (CreateSwapchain(description.Width, description.Height) == VdResult::SwapchainLost)
    {
        VdFail("Surface was unexpectedly lost.");
    }

    VkFenceCreateInfo fenceCI;
    fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCI.flags = 0;
    fenceCI.pNext = nullptr;
    vkCreateFence(_gd->GetVkDevice(), &fenceCI, nullptr, &_imageAvailableFence);

    AcquireNextImage(_gd->GetVkDevice(), VK_NULL_HANDLE, _imageAvailableFence);
    vkWaitForFences(_gd->GetVkDevice(), 1, &_imageAvailableFence, true, std::numeric_limits<uint64_t>::max());
    vkResetFences(_gd->GetVkDevice(), 1, &_imageAvailableFence);
}
Swapchain::~Swapchain()
{
    delete _framebuffer;
}

VdResult Swapchain::CreateSwapchain(uint32_t width, uint32_t height)
{
    if (_deviceSwapchain != VK_NULL_HANDLE)
    {
        _gd->WaitForIdle();
    }

    // Obtain the surface capabilities first -- this will indicate whether the surface has been lost.
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_gd->GetPhysicalDevice(), _surface, &surfaceCapabilities);
    if (result == VK_ERROR_SURFACE_LOST_KHR)
    {
        return VdResult::SwapchainLost;
    }

    _currentImageIndex = 0;
    uint32_t surfaceFormatCount = 0;
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(_gd->GetPhysicalDevice(), _surface, &surfaceFormatCount, nullptr);
    CheckResult(result);
    std::vector<VkSurfaceFormatKHR> formats(surfaceFormatCount);
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(_gd->GetPhysicalDevice(), _surface, &surfaceFormatCount, formats.data());
    CheckResult(result);

    VkSurfaceFormatKHR surfaceFormat = {};
    if (surfaceFormatCount == 1u && formats[0].format == VK_FORMAT_UNDEFINED)
    {
        surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        surfaceFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
    }
    else
    {
        for (auto& format : formats)
        {
            if (format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && format.format == VK_FORMAT_B8G8R8A8_UNORM)
            {
                surfaceFormat = format;
                break;
            }
        }
        if (surfaceFormat.format == VK_FORMAT_UNDEFINED)
        {
            surfaceFormat = formats[0];
        }
    }

    uint32_t presentModeCount = 0;
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(_gd->GetPhysicalDevice(), _surface, &presentModeCount, nullptr);
    CheckResult(result);
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(_gd->GetPhysicalDevice(), _surface, &presentModeCount, presentModes.data());
    CheckResult(result);

    std::unordered_set<VkPresentModeKHR> presentModesSet(presentModes.begin(), presentModes.end());

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;

    if (_syncToVBlank)
    {
        if (presentModesSet.count(VK_PRESENT_MODE_FIFO_RELAXED_KHR) != 0)
        {
            presentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
        }
    }
    else
    {
        if (presentModesSet.count(VK_PRESENT_MODE_MAILBOX_KHR) != 0)
        {
            presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        }
        else if (presentModesSet.count(VK_PRESENT_MODE_IMMEDIATE_KHR) != 0)
        {
            presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
    }

    uint32_t maxImageCount = surfaceCapabilities.maxImageCount == 0 ? UINT32_MAX : surfaceCapabilities.maxImageCount;
    uint32_t imageCount = std::min(maxImageCount, surfaceCapabilities.minImageCount + 1);

    VkSwapchainCreateInfoKHR swapchainCI = {};
    swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCI.surface = _surface;
    swapchainCI.presentMode = presentMode;
    swapchainCI.imageFormat = surfaceFormat.format;
    swapchainCI.imageColorSpace = surfaceFormat.colorSpace;
    uint32_t clampedWidth = std::clamp(width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
    uint32_t clampedHeight = std::clamp(height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
    swapchainCI.imageExtent.width = clampedWidth;
    swapchainCI.imageExtent.height = clampedHeight;
    swapchainCI.minImageCount = imageCount;
    swapchainCI.imageArrayLayers = 1;
    swapchainCI.imageUsage = VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    uint32_t queueFamilyIndices[] = { _gd->GetGraphicsQueueIndex(), _gd->GetPresentQueueIndex() };

    if (_gd->GetGraphicsQueueIndex() != _gd->GetPresentQueueIndex())
    {
        swapchainCI.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCI.queueFamilyIndexCount = 2;
        swapchainCI.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        swapchainCI.imageSharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
        swapchainCI.queueFamilyIndexCount = 0;
    }

    swapchainCI.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchainCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCI.clipped = true;

    VkSwapchainKHR oldSwapchain = _deviceSwapchain;
    swapchainCI.oldSwapchain = oldSwapchain;

    result = vkCreateSwapchainKHR(_gd->GetVkDevice(), &swapchainCI, nullptr, &_deviceSwapchain);
    CheckResult(result);
    if (oldSwapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(_gd->GetVkDevice(), oldSwapchain, nullptr);
    }

    _framebuffer->SetNewSwapchain(_deviceSwapchain, width, height, surfaceFormat, swapchainCI.imageExtent);

    return VdResult::Success;
}

bool Swapchain::GetPresentQueueIndex(uint32_t* queueFamilyIndex)
{
    uint32_t graphicsQueueIndex = _gd->GetGraphicsQueueIndex();
    uint32_t presentQueueIndex = _gd->GetPresentQueueIndex();

    if (QueueSupportsPresent(graphicsQueueIndex, _surface))
    {
        *queueFamilyIndex = graphicsQueueIndex;
        return true;
    }
    else if (graphicsQueueIndex != presentQueueIndex && QueueSupportsPresent(presentQueueIndex, _surface))
    {
        *queueFamilyIndex = presentQueueIndex;
        return true;
    }

    queueFamilyIndex = 0;
    return false;
}

bool Swapchain::QueueSupportsPresent(uint32_t queueFamilyIndex, VkSurfaceKHR surface)
{
    VkBool32 supported;
    VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(
        _gd->GetPhysicalDevice(),
        queueFamilyIndex,
        surface,
        &supported);
    CheckResult(result);
    return supported;
}

VD_EXPORT FramebufferBase* VdSwapchain_GetFramebuffer(Swapchain* sc)
{
    return sc->GetFramebuffer();
}

VD_EXPORT VdResult VdSwapchain_Resize(Swapchain* sc, uint32_t width, uint32_t height)
{
    return sc->Resize(width, height);
}
}