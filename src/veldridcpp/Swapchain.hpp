#pragma once
#include "SwapchainDescription.hpp"
#include "SwapchainFramebuffer.hpp"
#include "vulkan.h"
#include <optional>
#include <mutex>

namespace Veldrid
{
class Swapchain
{
public:
    Swapchain(GraphicsDevice* gd, const SwapchainDescription& description, VkSurfaceKHR existingSurface);
    ~Swapchain();
    void SwapBuffers() const;
    VdResult Resize(uint32_t width, uint32_t height);
    bool AcquireNextImage(VkDevice device, VkSemaphore semaphore, VkFence fence);
    VkSwapchainKHR DeviceSwapchain() const { return _deviceSwapchain; }
    uint32_t ImageIndex() const { return _currentImageIndex; }
    VkQueue PresentQueue() const { return _presentQueue; }
    uint32_t PresentQueueIndex() const { return _presentQueueIndex; }
    std::recursive_mutex& Mutex() { return _mutex; }
    VkFence ImageAvailableFence() const { return _imageAvailableFence; }
    FramebufferBase* GetFramebuffer() const { return _framebuffer; }

private:
    GraphicsDevice * _gd;
    VkSurfaceKHR _surface;
    VkSwapchainKHR _deviceSwapchain;
    SwapchainFramebuffer* _framebuffer;
    VkFence _imageAvailableFence;
    uint32_t _presentQueueIndex;
    VkQueue _presentQueue;
    bool _syncToVBlank;
    std::optional<bool> _newSyncToVBlank;
    uint32_t _currentImageIndex;
    std::recursive_mutex _mutex;

    VdResult CreateSwapchain(uint32_t width, uint32_t height);
    bool GetPresentQueueIndex(uint32_t* queueFamilyIndex);
    bool QueueSupportsPresent(uint32_t queueFamilyIndex, VkSurfaceKHR surface);
};
}
