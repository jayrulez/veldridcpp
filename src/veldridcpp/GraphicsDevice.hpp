#pragma once
#include "VeldridConfig.hpp"
#include "GraphicsDeviceCallbacks.hpp"
#include "vulkan.h"
#include "VdResult.hpp"
#include "MemoryManager.hpp"
#include "MapMode.hpp"
#include "MappedResource.hpp"
#include "PixelFormat.hpp"
#include "stdint.h"
#include <mutex>
#include <deque>
#include <unordered_set>

namespace Veldrid
{

struct GraphicsDeviceOptions;
class ResourceFactory;
class Swapchain;
class DeviceBuffer;
class Texture;
class Fence;
class CommandList;
class DescriptorPoolManager;

class GraphicsDevice
{
    class SharedCommandPool;

public:
    GraphicsDevice() { }
    VdResult Init(const GraphicsDeviceOptions& options, const GraphicsDeviceCallbacks& callbacks);
    ~GraphicsDevice();

    ResourceFactory* GetResourceFactory() const { return _factory; }
    VkDevice GetVkDevice() const { return _device; }
    VkInstance GetInstance() const { return _instance; }
    VkPhysicalDevice GetPhysicalDevice() const { return _physicalDevice; }
    VkPhysicalDeviceProperties GetPhysicalDeviceProperties() const { return _physicalDeviceProperties; }
    VkPhysicalDeviceMemoryProperties GetPhysicalDeviceMemProperties() const { return _physicalDeviceMemProperties; }
    const GraphicsDeviceCallbacks& GetGraphicsDeviceCallbacks() const { return _callbacks; }
    MemoryManager& GetMemoryManager() { return _memoryManager; }
    DescriptorPoolManager& GetDescriptorPoolManager() { return *_descriptorPoolManager; }

    VdResult SwapBuffers(Swapchain& sc);
    VdResult UpdateBuffer(DeviceBuffer* buffer, uint32_t bufferOffsetInBytes, void* source, uint32_t sizeInBytes);
    VdResult UpdateTexture(Texture* texture, void* source, uint32_t sizeInBytes, uint32_t x, uint32_t y, uint32_t z, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevel, uint32_t arrayLayer);
    VdResult SubmitCommands(CommandList* cl, Fence* fence);
    VdResult MapBuffer(DeviceBuffer* buffer, MapMode mode, MappedResource* mappedResource);
    VdResult UnmapBuffer(DeviceBuffer* buffer);
    VdResult MapTexture(Texture* texture, MapMode mode, uint32_t subresource, MappedResource* mappedResource);
    VdResult UnmapTexture(Texture* texture, uint32_t subresource);

    VdResult WaitForIdle();

    uint32_t GetGraphicsQueueIndex() { return _graphicsQueueIndex; }
    uint32_t GetPresentQueueIndex() { return _presentQueueIndex; }

    void ClearColorTexture(Texture* texture, VkClearColorValue color);
    void ClearDepthTexture(Texture* texture, VkClearDepthStencilValue value);
    void EnqueueDisposedCommandBuffer(CommandList* cl);

private:
    bool _debug;
    GraphicsDeviceCallbacks _callbacks;
    ResourceFactory* _factory;
    VkDevice _device;
    VkInstance _instance;
    VkPhysicalDevice _physicalDevice;
    VkPhysicalDeviceProperties _physicalDeviceProperties;
    VkPhysicalDeviceFeatures _physicalDeviceFeatures;
    VkPhysicalDeviceMemoryProperties _physicalDeviceMemProperties;
    MemoryManager _memoryManager;
    DescriptorPoolManager* _descriptorPoolManager;

    std::recursive_mutex _commandListsToDisposeLock;
    std::unordered_set<CommandList*> _commandListsToDispose;

    VkDebugReportCallbackEXT _debugLayerCallback;
    bool _debugMarkerEnabled;
    PFN_vkDebugMarkerSetObjectNameEXT _setObjectNamePtr;

    // Queue stuff
    std::recursive_mutex _graphicsQueueLock;
    std::recursive_mutex _submittedFencesLock;
    uint32_t _graphicsQueueIndex;
    uint32_t _presentQueueIndex;
    VkQueue _graphicsQueue;
    VkQueue _presentQueue;

    // Cached resources
    static const uint32_t MinStagingBufferSize = 256;
    static const uint32_t MaxStagingBufferSize = 512;
    std::recursive_mutex _stagingResourcesLock;
    std::vector<DeviceBuffer*> _availableStagingBuffers;
    std::vector<Texture*> _availableStagingTextures;
    std::recursive_mutex _graphicsCommandPoolLock;
    std::vector<SharedCommandPool*> _availableSharedCommandPools;
    std::unordered_map<VkCommandBuffer, SharedCommandPool*> _submittedSharedCommandPools;
    std::unordered_map<VkCommandBuffer, DeviceBuffer*> _submittedStagingBuffers;
    std::unordered_map<VkCommandBuffer, Texture*> _submittedStagingTextures;

    std::recursive_mutex _submissionFencesLock;
    std::deque<VkFence> _availableSubmissionFences;
    std::unordered_map<VkFence, std::tuple<CommandList*, VkCommandBuffer>> _submittedFences;
    std::vector<std::pair<VkFence, std::tuple<CommandList*, VkCommandBuffer>>> _completedFences;

    VdResult CreateInstance();
    VdResult CreatePhysicalDevice();
    void GetQueueFamilyIndices(VkSurfaceKHR surface);
    VdResult CreateLogicalDevice(VkSurfaceKHR surface);
    void CheckSubmittedFences();
    DeviceBuffer* GetFreeStagingBuffer(uint32_t size);
    Texture* GetFreeStagingTexture(uint32_t width, uint32_t height, uint32_t depth, PixelFormat format);
    SharedCommandPool* GetFreeCommandPool();
    VkFence GetFreeSubmissionFence();

    void SubmitCommandBuffer(
        CommandList* commandList,
        VkCommandBuffer vkCB,
        uint32_t waitSemaphoreCount,
        VkSemaphore* waitSemaphoresPtr,
        uint32_t signalSemaphoreCount,
        VkSemaphore* signalSemaphoresPtr,
        Fence* fence);

    class SharedCommandPool
    {
    private:
        GraphicsDevice * _gd;
        VkCommandPool _pool;
        VkCommandBuffer _cb;
        bool _isCached;

    public:
        bool IsCached() const { return _isCached; }

        SharedCommandPool(GraphicsDevice* gd, bool isCached)
        {
            _gd = gd;
            _isCached = isCached;

            VkCommandPoolCreateInfo commandPoolCI = {};
            commandPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            commandPoolCI.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            commandPoolCI.queueFamilyIndex = _gd->_graphicsQueueIndex;
            CheckResult(vkCreateCommandPool(_gd->GetVkDevice(), &commandPoolCI, nullptr, &_pool));

            VkCommandBufferAllocateInfo allocateInfo = {};
            allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocateInfo.commandBufferCount = 1;
            allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocateInfo.commandPool = _pool;
            VkResult result = vkAllocateCommandBuffers(_gd->GetVkDevice(), &allocateInfo, &_cb);
            CheckResult(result);
        }

        ~SharedCommandPool()
        {
            vkDestroyCommandPool(_gd->GetVkDevice(), _pool, nullptr);
        }

        VkCommandBuffer BeginNewCommandBuffer()
        {
            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            CheckResult(vkBeginCommandBuffer(_cb, &beginInfo));

            return _cb;
        }

        void EndAndSubmit(VkCommandBuffer cb)
        {
            CheckResult(vkEndCommandBuffer(cb));
            _gd->SubmitCommandBuffer(nullptr, cb, 0, nullptr, 0, nullptr, nullptr);
            _gd->_stagingResourcesLock.lock();
            _gd->_submittedSharedCommandPools[cb] = this;
            _gd->_stagingResourcesLock.unlock();
        }

        void Reset()
        {
            CheckResult(vkResetCommandPool(_gd->GetVkDevice(), _pool, 0));
        }
    };
};

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objType,
    uint64_t obj,
    size_t location,
    int32_t code,
    const char* layerPrefix,
    const char* msg,
    void* userData);
}