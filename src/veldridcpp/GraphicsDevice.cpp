#include "stdafx.h"
#include "GraphicsDevice.hpp"
#include "GraphicsDeviceOptions.hpp"
#include "DescriptorPoolManager.hpp"
#include "Swapchain.hpp"
#include "VeldridConfig.hpp"
#include "VulkanUtil.hpp"
#include "vulkan.h"
#include <unordered_set>
#include <iostream>
#include "VdResult.hpp"
#include "MemoryManager.hpp"
#include "ResourceFactory.hpp"
#include "Fence.hpp"
#include "FormatHelpers.hpp"
#include "Util.hpp"
#include <cassert>
#include <mutex>
#include <algorithm>

#ifdef _WINDOWS
#include "vulkan_win32.h"
#endif

namespace Veldrid
{
VdResult GraphicsDevice::Init(const GraphicsDeviceOptions& options, const GraphicsDeviceCallbacks& callbacks)
{
    _debug = options.Debug;
    _callbacks = callbacks;
    VdResult result = CreateInstance();
    if (result != VdResult::Success) { return result; }

    result = CreatePhysicalDevice();
    if (result != VdResult::Success) { return result; }

    result = CreateLogicalDevice(VK_NULL_HANDLE);
    if (result != VdResult::Success) { return result; }

    _memoryManager.Init(_device, _physicalDevice);
    _factory = new ResourceFactory(this);

    _descriptorPoolManager = new DescriptorPoolManager(this);

    return VdResult::Success;
}

GraphicsDevice::~GraphicsDevice()
{
    delete _descriptorPoolManager;
    // TODO: Destroy stuff.
}

VdResult GraphicsDevice::CreateInstance()
{
    uint32_t propCount;
    CheckResult(vkEnumerateInstanceLayerProperties(&propCount, nullptr));
    std::vector<VkLayerProperties> layerProperties(propCount);
    CheckResult(vkEnumerateInstanceLayerProperties(&propCount, layerProperties.data()));

    std::unordered_set<std::string> availableInstanceLayers;
    for (auto prop : layerProperties)
    {
        availableInstanceLayers.insert(prop.layerName);
    }

    uint32_t extensionCount;
    CheckResult(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr));
    std::vector<VkExtensionProperties> extensionProperties(extensionCount);
    CheckResult(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensionProperties.data()));

    std::unordered_set<std::string> availableInstanceExtensions;
    for (auto prop : extensionProperties)
    {
        std::string(prop.extensionName);
        availableInstanceExtensions.insert(prop.extensionName);
    }

    VkInstanceCreateInfo instanceCI;
    instanceCI.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

    VkApplicationInfo applicationInfo;
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.pApplicationName = "Veldrid Native Vulkan Backend";
    applicationInfo.pEngineName = "Veldrid Native Vulkan Backend";

    instanceCI.pApplicationInfo = &applicationInfo;

    std::vector<const char*> instanceExtensions;
    std::vector<const char*> instanceLayers;

    if (availableInstanceExtensions.count(VK_KHR_SURFACE_EXTENSION_NAME) == 0)
    {
        return VdResult::UnsupportedSystem;
    }

    instanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

#ifdef _WINDOWS
    if (availableInstanceExtensions.count(VK_KHR_WIN32_SURFACE_EXTENSION_NAME) == 0)
    {
        return VdResult::UnsupportedSystem;
    }
    instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#else
#error Unsupported Platform
#endif

    bool debugReportExtensionAvailable = false;
    if (_debug)
    {
        if (availableInstanceExtensions.count(VK_EXT_DEBUG_REPORT_EXTENSION_NAME) != 0)
        {
            debugReportExtensionAvailable = true;
            instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        }
        if (availableInstanceLayers.count("VK_LAYER_LUNARG_standard_validation") != 0)
        {
            instanceLayers.push_back("VK_LAYER_LUNARG_standard_validation");
        }
    }

    instanceCI.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
    instanceCI.ppEnabledExtensionNames = instanceExtensions.data();

    instanceCI.enabledLayerCount = static_cast<uint32_t>(instanceLayers.size());
    instanceCI.ppEnabledLayerNames = instanceLayers.data();
    instanceCI.flags = 0;
    instanceCI.pNext = nullptr;
    CheckResult(vkCreateInstance(&instanceCI, nullptr, &_instance));

    if (_debug && debugReportExtensionAvailable)
    {
        VkDebugReportCallbackCreateInfoEXT callbackCI = {};
        callbackCI.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT;
        callbackCI.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        callbackCI.pfnCallback = DebugCallback;

        auto createCallbackFunc = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(_instance, "vkCreateDebugReportCallbackEXT");
        CheckResult(createCallbackFunc(_instance, &callbackCI, nullptr, &_debugLayerCallback));
    }

    return VdResult::Success;
}

VdResult GraphicsDevice::CreatePhysicalDevice()
{
    uint32_t deviceCount = 0;
    CheckResult(vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr));
    if (deviceCount == 0)
    {
        return VdResult::UnsupportedSystem;
    }

    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    CheckResult(vkEnumeratePhysicalDevices(_instance, &deviceCount, physicalDevices.data()));
    // Just use the first one.
    _physicalDevice = physicalDevices.front();
    vkGetPhysicalDeviceProperties(_physicalDevice, &_physicalDeviceProperties);
    vkGetPhysicalDeviceFeatures(_physicalDevice, &_physicalDeviceFeatures);
    vkGetPhysicalDeviceMemoryProperties(_physicalDevice, &_physicalDeviceMemProperties);

    return VdResult::Success;
}

VdResult GraphicsDevice::CreateLogicalDevice(VkSurfaceKHR surface)
{
    GetQueueFamilyIndices(surface);

    std::unordered_set<uint32_t> familyIndices = std::unordered_set<uint32_t>{ _graphicsQueueIndex, _presentQueueIndex };
    uint32_t queueCount = static_cast<uint32_t>(familyIndices.size());
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos = std::vector<VkDeviceQueueCreateInfo>(queueCount);

    uint32_t i = 0;
    for (uint32_t queueIndex : familyIndices)
    {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = _graphicsQueueIndex;
        queueCreateInfo.queueCount = 1;
        float priority = 1.f;
        queueCreateInfo.pQueuePriorities = &priority;
        queueCreateInfos[i] = queueCreateInfo;
        i += 1;
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = true;
    deviceFeatures.fillModeNonSolid = true;
    deviceFeatures.geometryShader = true;
    deviceFeatures.depthClamp = true;
    deviceFeatures.multiViewport = true;
    deviceFeatures.textureCompressionBC = true;

    bool debugMarkerSupported = false;

    uint32_t propertyCount;
    CheckResult(vkEnumerateDeviceExtensionProperties(_physicalDevice, nullptr, &propertyCount, nullptr));
    auto properties = std::vector<VkExtensionProperties>(propertyCount);
    CheckResult(vkEnumerateDeviceExtensionProperties(_physicalDevice, nullptr, &propertyCount, properties.data()));

    for (uint32_t property = 0; property < propertyCount; property++)
    {
        if (strcmp(properties[property].extensionName, "VK_EXT_debug_marker") == 0)
        {
            debugMarkerSupported = true;
            break;
        }
    }

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = queueCount;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();

    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    std::vector<const char*> layerNames;
    layerNames.push_back("VK_LAYER_LUNARG_standard_validation");

    deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(layerNames.size());
    deviceCreateInfo.ppEnabledLayerNames = layerNames.data();

    std::vector<const char*> extensionNames;
    extensionNames.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    if (debugMarkerSupported)
    {
        extensionNames.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
        _debugMarkerEnabled = true;
    }
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensionNames.size());
    deviceCreateInfo.ppEnabledExtensionNames = extensionNames.data();

    CheckResult(vkCreateDevice(_physicalDevice, &deviceCreateInfo, nullptr, &_device));

    vkGetDeviceQueue(_device, _graphicsQueueIndex, 0, &_graphicsQueue);

    if (debugMarkerSupported)
    {
        _setObjectNamePtr = (PFN_vkDebugMarkerSetObjectNameEXT)vkGetInstanceProcAddr(_instance, "vkDebugMarkerSetObjectNameEXT");
    }

    return VdResult::Success;
}

void GraphicsDevice::GetQueueFamilyIndices(VkSurfaceKHR surface)
{
    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> qfp = std::vector<VkQueueFamilyProperties>(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queueFamilyCount, qfp.data());

    bool foundGraphics = false;
    bool foundPresent = surface == VK_NULL_HANDLE;

    for (uint32_t i = 0; i < qfp.size(); i++)
    {
        if ((qfp[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
        {
            _graphicsQueueIndex = i;
        }

        if (!foundPresent)
        {
            VkBool32 presentSupported;
            vkGetPhysicalDeviceSurfaceSupportKHR(_physicalDevice, i, surface, &presentSupported);
            if (presentSupported)
            {
                _presentQueueIndex = i;
                foundPresent = true;
            }
        }

        if (foundGraphics && foundPresent)
        {
            return;
        }
    }
}
static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objType,
    uint64_t obj,
    size_t location,
    int32_t code,
    const char* layerPrefix,
    const char* msg,
    void* userData)
{
    std::cerr << "validation layer: " << msg << std::endl;
    return VK_FALSE;
}

VdResult GraphicsDevice::SwapBuffers(Swapchain& sc)
{
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    VkSwapchainKHR deviceSwapchain = sc.DeviceSwapchain();
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &deviceSwapchain;
    uint32_t imageIndex = sc.ImageIndex();
    presentInfo.pImageIndices = &imageIndex;

    std::recursive_mutex& presentLock = sc.PresentQueueIndex() == _graphicsQueueIndex ? _graphicsQueueLock : sc.Mutex();
    presentLock.lock();
    {
        vkQueuePresentKHR(sc.PresentQueue(), &presentInfo);
        if (sc.AcquireNextImage(_device, VK_NULL_HANDLE, sc.ImageAvailableFence()))
        {
            VkFence fence = sc.ImageAvailableFence();
            vkWaitForFences(_device, 1, &fence, true, UINT64_MAX);
            vkResetFences(_device, 1, &fence);
        }
    }
    presentLock.unlock();

    return VdResult::Success;
}

VdResult GraphicsDevice::UpdateBuffer(
    DeviceBuffer* buffer,
    uint32_t bufferOffsetInBytes,
    void* source,
    uint32_t sizeInBytes)
{
    DeviceBuffer* copySrcVkBuffer = nullptr;
    void* mappedPtr;
    void* destPtr = nullptr;
    bool isPersistentMapped = buffer->GetMemory().IsPersistentMapped();
    if (isPersistentMapped)
    {
        mappedPtr = buffer->GetMemory().BlockMappedPointer();
        destPtr = (uint8_t*)mappedPtr + bufferOffsetInBytes;
    }
    else
    {
        copySrcVkBuffer = GetFreeStagingBuffer(sizeInBytes);
        mappedPtr = copySrcVkBuffer->GetMemory().BlockMappedPointer();
        destPtr = (uint8_t*)mappedPtr;
    }

    memcpy(destPtr, source, sizeInBytes);

    if (!isPersistentMapped)
    {
        SharedCommandPool* pool = GetFreeCommandPool();
        VkCommandBuffer cb = pool->BeginNewCommandBuffer();

        VkBufferCopy copyRegion;

        copyRegion.dstOffset = bufferOffsetInBytes;
        copyRegion.size = sizeInBytes;
        copyRegion.srcOffset = 0;
        vkCmdCopyBuffer(cb, copySrcVkBuffer->GetVkBuffer(), buffer->GetVkBuffer(), 1, &copyRegion);

        pool->EndAndSubmit(cb);
        _stagingResourcesLock.lock();
        _submittedStagingBuffers.emplace(cb, copySrcVkBuffer);
        _stagingResourcesLock.unlock();
    }

    return VdResult::Success;
}

VdResult GraphicsDevice::UpdateTexture(
    Texture* texture,
    void* source, uint32_t sizeInBytes,
    uint32_t x, uint32_t y, uint32_t z,
    uint32_t width, uint32_t height, uint32_t depth,
    uint32_t mipLevel, uint32_t arrayLayer)
{
    bool isStaging = (texture->GetUsage() & TextureUsage::Staging) == TextureUsage::Staging;
    if (isStaging)
    {
        const MemoryBlock& memBlock = texture->GetMemory();
        uint32_t subresource = texture->CalculateSubresource(mipLevel, arrayLayer);
        VkSubresourceLayout layout = texture->GetSubresourceLayout(subresource);
        uint8_t* imageBasePtr = memBlock.BlockMappedPointer() + layout.offset;

        uint32_t srcRowPitch = GetRowPitch(width, texture->GetFormat());
        uint32_t srcDepthPitch = GetDepthPitch(srcRowPitch, height, texture->GetFormat());
        CopyTextureRegion(
            source,
            0, 0, 0,
            srcRowPitch, srcDepthPitch,
            imageBasePtr,
            x, y, z,
            (uint32_t)layout.rowPitch, (uint32_t)layout.depthPitch,
            width, height, depth,
            texture->GetFormat());
    }
    else
    {
        Texture* stagingTex = GetFreeStagingTexture(width, height, depth, texture->GetFormat());
        UpdateTexture(stagingTex, source, sizeInBytes, 0, 0, 0, width, height, depth, 0, 0);
        SharedCommandPool* pool = GetFreeCommandPool();
        VkCommandBuffer cb = pool->BeginNewCommandBuffer();
        CommandList::CopyTextureCore_CommandBuffer(
            cb,
            stagingTex, 0, 0, 0, 0, 0,
            texture, x, y, z, mipLevel, arrayLayer,
            width, height, depth, 1);
        pool->EndAndSubmit(cb);
        _stagingResourcesLock.lock();
        _submittedStagingTextures.emplace(cb, stagingTex);
        _stagingResourcesLock.unlock();
    }

    return VdResult::Success;
}

VdResult GraphicsDevice::SubmitCommands(CommandList* cl, Fence* fence)
{
    SubmitCommandBuffer(
        cl,
        cl->GetVkCommandBuffer(),
        0, nullptr,
        0, nullptr,
        fence);
    cl->CommandBufferSubmitted();

    return VdResult::Success;
}

VdResult GraphicsDevice::MapBuffer(DeviceBuffer* buffer, MapMode mode, MappedResource* mappedResource)
{
    mappedResource->Mode = mode;

    MemoryBlock memoryBlock = buffer->GetMemory();
    mappedResource->SizeInBytes = buffer->GetSizeInBytes();
    mappedResource->Data = memoryBlock.BlockMappedPointer();

    return VdResult::Success;
}

VdResult GraphicsDevice::UnmapBuffer(DeviceBuffer* buffer)
{
    return VdResult::Success;
}

VdResult GraphicsDevice::MapTexture(Texture* texture, MapMode mode, uint32_t subresource, MappedResource* mappedResource)
{
    mappedResource->Mode = mode;
    mappedResource->Subresource = subresource;

    VkSubresourceLayout layout = texture->GetSubresourceLayout(subresource);
    const MemoryBlock& memoryBlock = texture->GetMemory();
    mappedResource->SizeInBytes = static_cast<uint32_t>(layout.size);
    mappedResource->RowPitch = static_cast<uint32_t>(layout.rowPitch);
    mappedResource->DepthPitch = static_cast<uint32_t>(layout.depthPitch);
    mappedResource->Data = (memoryBlock.BlockMappedPointer() + layout.offset);

    return VdResult::Success;
}

VdResult GraphicsDevice::UnmapTexture(Texture * texture, uint32_t subresource)
{
    return VdResult::Success;
}

GraphicsDevice::SharedCommandPool* GraphicsDevice::GetFreeCommandPool()
{
    _stagingResourcesLock.lock();
    GraphicsDevice::SharedCommandPool* result;
    if (_availableSharedCommandPools.size() > 0)
    {
        result = _availableSharedCommandPools[_availableSharedCommandPools.size() - 1];
        _availableSharedCommandPools.pop_back();
    }
    else
    {
        result = new GraphicsDevice::SharedCommandPool(this, true);
    }
    _stagingResourcesLock.unlock();

    return result;
}

VkFence GraphicsDevice::GetFreeSubmissionFence()
{
    _submissionFencesLock.lock();
    if (_availableSubmissionFences.size() > 0)
    {
        auto fence = _availableSubmissionFences.front();
        _availableSubmissionFences.pop_front();
        _submissionFencesLock.unlock();
        return fence;
    }
    _submissionFencesLock.unlock();

    VkFence ret;
    VkFenceCreateInfo fenceCI;
    fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCI.flags = 0;
    fenceCI.pNext = nullptr;
    CheckResult(vkCreateFence(_device, &fenceCI, nullptr, &ret));

    return ret;
}

void GraphicsDevice::SubmitCommandBuffer(
    CommandList* commandList,
    VkCommandBuffer vkCB,
    uint32_t waitSemaphoreCount,
    VkSemaphore* waitSemaphoresPtr,
    uint32_t signalSemaphoreCount,
    VkSemaphore* signalSemaphoresPtr,
    Fence* fence)
{
    CheckSubmittedFences();

    bool useExtraFence = fence != nullptr;

    VkSubmitInfo si = {};
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &vkCB;
    VkPipelineStageFlags waitDstStageMask = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    si.pWaitDstStageMask = &waitDstStageMask;

    si.pWaitSemaphores = waitSemaphoresPtr;
    si.waitSemaphoreCount = waitSemaphoreCount;
    si.pSignalSemaphores = signalSemaphoresPtr;
    si.signalSemaphoreCount = signalSemaphoreCount;

    VkFence vkFence = VK_NULL_HANDLE;
    VkFence submissionFence = VK_NULL_HANDLE;
    if (useExtraFence)
    {
        vkFence = fence->GetVkFence();
        submissionFence = GetFreeSubmissionFence();
    }
    else
    {
        vkFence = GetFreeSubmissionFence();
        submissionFence = vkFence;
    }

    _graphicsQueueLock.lock();
    vkQueueSubmit(_graphicsQueue, 1, &si, vkFence);
    _submittedFences[submissionFence] = std::tuple<CommandList*, VkCommandBuffer>(commandList, vkCB);

    if (useExtraFence)
    {
        vkQueueSubmit(_graphicsQueue, 0, nullptr, submissionFence);
    }

    _graphicsQueueLock.unlock();
}

void GraphicsDevice::CheckSubmittedFences()
{
    _submittedFencesLock.lock();
    {
        VdAssert(_completedFences.size() == 0);
        for (auto& kvp : _submittedFences)
        {
            if (vkGetFenceStatus(_device, kvp.first) == VK_SUCCESS)
            {
                _completedFences.push_back(kvp);
            }
        }

        for (auto& kvp : _completedFences)
        {
            VkFence fence = kvp.first;
            VkCommandBuffer completedCB = std::get<1>(kvp.second);
            CommandList* completedCL = std::get<0>(kvp.second);
            if (completedCL != nullptr)
            {
                completedCL->CommandBufferCompleted(completedCB);
            }

            bool result = _submittedFences.erase(fence) != 0;
            VdAssert(result);
            VkResult resetResult = vkResetFences(_device, 1, &fence);
            CheckResult(resetResult);
            _availableSubmissionFences.push_back(fence);
            _stagingResourcesLock.lock();
            {
                auto texI = _submittedStagingTextures.find(completedCB);
                if (texI != _submittedStagingTextures.end())
                {
                    _availableStagingTextures.push_back(texI->second);
                    _submittedStagingTextures.erase(texI);
                }

                auto bufferI = _submittedStagingBuffers.find(completedCB);
                if (bufferI != _submittedStagingBuffers.end())
                {
                    auto stagingBuffer = bufferI->second;
                    if (stagingBuffer->GetSizeInBytes() <= MaxStagingBufferSize)
                    {
                        _availableStagingBuffers.push_back(stagingBuffer);
                    }
                    else
                    {
                        delete stagingBuffer;
                    }

                    _submittedStagingBuffers.erase(bufferI);
                }

                auto commandPoolI = _submittedSharedCommandPools.find(completedCB);
                if (commandPoolI != _submittedSharedCommandPools.end())
                {
                    _graphicsCommandPoolLock.lock();
                    {
                        auto sharedPool = commandPoolI->second;
                        if (sharedPool->IsCached())
                        {
                            _availableSharedCommandPools.push_back(sharedPool);
                        }
                        else
                        {
                            delete sharedPool;
                        }
                    }
                    _graphicsCommandPoolLock.unlock();

                    _submittedSharedCommandPools.erase(commandPoolI);
                }
            }
            _stagingResourcesLock.unlock();

            if (completedCL != nullptr)
            {
                _commandListsToDisposeLock.lock();
                {
                    if (completedCL->GetSubmissionCount() == 0)
                    {
                        if (_commandListsToDispose.erase(completedCL) != 0)
                        {
                            delete completedCL;
                        }
                    }
                }
                _commandListsToDisposeLock.unlock();
            }
        }

        _completedFences.clear();
    }
    _submittedFencesLock.unlock();
}

DeviceBuffer* GraphicsDevice::GetFreeStagingBuffer(uint32_t size)
{
    _stagingResourcesLock.lock();
    for (uint32_t i = 0; i < _availableStagingBuffers.size(); i++)
    {
        DeviceBuffer* buffer = _availableStagingBuffers[i];
        if (buffer->GetSizeInBytes() >= size)
        {
            _availableStagingBuffers.erase(_availableStagingBuffers.begin() + i);
            _stagingResourcesLock.unlock();
            return buffer;
        }
    }
    _stagingResourcesLock.unlock();

    uint32_t newBufferSize = std::max(MinStagingBufferSize, size);
    DeviceBuffer* newBuffer = _factory->CreateBuffer(
        BufferDescription(newBufferSize, BufferUsage::Staging));
    return newBuffer;
}

Texture* GraphicsDevice::GetFreeStagingTexture(uint32_t width, uint32_t height, uint32_t depth, PixelFormat format)
{
    uint32_t pixelSize = GetSizeInBytes(format);
    uint32_t totalSize = width * height * depth * pixelSize;
    _stagingResourcesLock.lock();
    for (int i = 0; i < _availableStagingTextures.size(); i++)
    {
        Texture* tex = _availableStagingTextures[i];
        if (tex->GetMemory().Size >= totalSize)
        {
            _availableStagingTextures.erase(_availableStagingTextures.begin() + i);
            tex->SetStagingDimensions(width, height, depth, format);
            _stagingResourcesLock.unlock();
            return tex;
        }
    }
    _stagingResourcesLock.unlock();

    uint32_t texWidth = std::max(256u, width);
    uint32_t texHeight = std::max(256u, height);
    Texture* newTex = _factory->CreateTexture(
        TextureDescription::Texture3D(texWidth, texHeight, depth, 1, format, TextureUsage::Staging));
    newTex->SetStagingDimensions(width, height, depth, format);

    return newTex;
}

VdResult GraphicsDevice::WaitForIdle()
{
    _graphicsQueueLock.lock();
    CheckResult(vkQueueWaitIdle(_graphicsQueue));
    _graphicsQueueLock.unlock();

    return VdResult::Success;
}

void GraphicsDevice::ClearColorTexture(Texture * texture, VkClearColorValue color)
{
    VkImageSubresourceRange range;
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel = 0;
    range.levelCount = texture->GetMipLevels();
    range.baseArrayLayer = 0;
    range.layerCount = texture->GetArrayLayers();
    SharedCommandPool* pool = GetFreeCommandPool();
    VkCommandBuffer cb = pool->BeginNewCommandBuffer();
    texture->TransitionImageLayout(cb, 0, texture->GetMipLevels(), 0, texture->GetArrayLayers(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    vkCmdClearColorImage(cb, texture->GetOptimalImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &color, 1, &range);
    texture->TransitionImageLayout(cb, 0, texture->GetMipLevels(), 0, texture->GetArrayLayers(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    pool->EndAndSubmit(cb);
}

void GraphicsDevice::ClearDepthTexture(Texture * texture, VkClearDepthStencilValue clearValue)
{
    VkImageSubresourceRange range;
    range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    range.baseMipLevel = 0;
    range.levelCount = texture->GetMipLevels();
    range.baseArrayLayer = 0;
    range.layerCount = texture->GetArrayLayers();
    SharedCommandPool* pool = GetFreeCommandPool();
    VkCommandBuffer cb = pool->BeginNewCommandBuffer();
    texture->TransitionImageLayout(cb, 0, texture->GetMipLevels(), 0, texture->GetArrayLayers(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    vkCmdClearDepthStencilImage(
        cb,
        texture->GetOptimalImage(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        &clearValue,
        1,
        &range);
    texture->TransitionImageLayout(cb, 0, texture->GetMipLevels(), 0, texture->GetArrayLayers(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    pool->EndAndSubmit(cb);
}

void GraphicsDevice::EnqueueDisposedCommandBuffer(CommandList* cl)
{
    _commandListsToDisposeLock.lock();
    _commandListsToDispose.insert(cl);
    _commandListsToDisposeLock.unlock();
}

VD_EXPORT VdResult VdGraphicsDevice_CreateVulkan(
    GraphicsDeviceOptions* options,
    GraphicsDeviceCallbacks* callbacks,
    GraphicsDevice** graphicsDevice)
{
    GraphicsDevice* gd = new GraphicsDevice();
    auto result = gd->Init(*options, *callbacks);
    if (result == VdResult::Success)
    {
        *graphicsDevice = gd;
    }

    return result;
}

VD_EXPORT ResourceFactory* VdGraphicsDevice_GetResourceFactory(GraphicsDevice* gd)
{
    return gd->GetResourceFactory();
}

VD_EXPORT VdResult VdGraphicsDevice_SwapBuffers(GraphicsDevice* gd, Swapchain* sc)
{
    return gd->SwapBuffers(*sc);
}

VD_EXPORT VdResult VdGraphicsDevice_UpdateBuffer(
    GraphicsDevice* gd,
    DeviceBuffer* buffer,
    uint32_t bufferOffsetInBytes,
    void* source,
    uint32_t sizeInBytes)
{
    return gd->UpdateBuffer(buffer, bufferOffsetInBytes, source, sizeInBytes);
}

VD_EXPORT VdResult VdGraphicsDevice_SubmitCommands(
    GraphicsDevice* gd,
    CommandList* cl,
    Fence* fence)
{
    return gd->SubmitCommands(cl, fence);
}

VD_EXPORT VdResult VdGraphicsDevice_MapBuffer(
    GraphicsDevice* gd,
    DeviceBuffer* buffer,
    MapMode mode,
    MappedResource* mappedResource)
{
    return gd->MapBuffer(buffer, mode, mappedResource);
}

VD_EXPORT VdResult VdGraphicsDevice_UnmapBuffer(GraphicsDevice* gd, DeviceBuffer* buffer)
{
    return gd->UnmapBuffer(buffer);
}

VD_EXPORT VdResult VdGraphicsDevice_MapTexture(
    GraphicsDevice* gd,
    Texture* texture,
    MapMode mode,
    uint32_t subresource,
    MappedResource* mappedResource)
{
    return gd->MapTexture(texture, mode, subresource, mappedResource);
}

VD_EXPORT VdResult VdGraphicsDevice_UnmapTexture(GraphicsDevice* gd, Texture* texture, uint32_t subresource)
{
    return gd->UnmapTexture(texture, subresource);
}

VD_EXPORT VdResult VdGraphicsDevice_UpdateTexture(
    GraphicsDevice* gd,
    Texture* texture,
    void* source, uint32_t sizeInBytes,
    uint32_t x, uint32_t y, uint32_t z,
    uint32_t width, uint32_t height, uint32_t depth,
    uint32_t mipLevel, uint32_t arrayLayer)
{
    return gd->UpdateTexture(
        texture,
        source, sizeInBytes,
        x, y, z,
        width, height, depth,
        mipLevel, arrayLayer);
}

VD_EXPORT VdResult VdGraphicsDevice_WaitForIdle(GraphicsDevice* gd)
{
    return gd->WaitForIdle();
}

VD_EXPORT VdResult VdGraphicsDevice_Dispose(GraphicsDevice* gd)
{
    delete gd;
    return VdResult::Success;
}
}
