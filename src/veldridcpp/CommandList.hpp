#pragma once
#include "vulkan.h"
#include "GraphicsDevice.hpp"
#include "Pipeline.hpp"
#include "Framebuffer.hpp"
#include "RgbaFloat.hpp"
#include "ResourceSet.hpp"
#include <mutex>
#include <deque>
#include <optional>

namespace Veldrid
{
class CommandList
{
public:
    CommandList(GraphicsDevice* gd);
    VdResult Begin();
    VdResult End();
    VdResult Dispose();
    VdResult UpdateBuffer(DeviceBuffer* buffer, uint32_t offset, void* source, uint32_t size);
    VdResult CopyBuffer(DeviceBuffer* source, uint32_t sourceOffset, DeviceBuffer* destination, uint32_t destinationOffset, uint32_t size);
    VdResult CopyTexture(
        Texture* source,
        uint32_t srcX, uint32_t srcY, uint32_t srcZ,
        uint32_t srcMipLevel,
        uint32_t srcBaseArrayLayer,
        Texture* destination,
        uint32_t dstX, uint32_t dstY, uint32_t dstZ,
        uint32_t dstMipLevel,
        uint32_t dstBaseArrayLayer,
        uint32_t width, uint32_t height, uint32_t depth,
        uint32_t layerCount);
    VdResult SetFramebuffer(Framebuffer* fb);

    VdResult SetViewport(uint32_t index, VkViewport* viewport);
    VdResult SetScissorRect(uint32_t index, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
    VdResult ClearColorTarget(uint32_t index, RgbaFloat clearColor);
    VdResult ClearDepthStencil(float depth, uint8_t stencil);

    VdResult SetPipeline(Pipeline* pipeline);
    VdResult SetVertexBuffer(uint32_t index, DeviceBuffer* buffer);
    VdResult SetIndexBuffer(DeviceBuffer* buffer, IndexFormat format);
    VdResult SetGraphicsResourceSet(uint32_t slot, ResourceSet* rs);
    VdResult Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t vertexStart, uint32_t instanceStart);
    VdResult DrawIndexed(
        uint32_t indexCount,
        uint32_t instanceCount,
        uint32_t indexStart,
        int32_t vertexOffset,
        uint32_t instanceStart);
    void SetFullScissorRects();

    void CommandBufferSubmitted() { _submittedCommandBufferCount += 1; }
    void CommandBufferCompleted(VkCommandBuffer cb);
    uint32_t GetSubmissionCount() const { return _submittedCommandBufferCount; }

    VkCommandBuffer GetVkCommandBuffer() { return _cb; }

    static void CopyTextureCore_CommandBuffer(
        VkCommandBuffer cb,
        Texture* source,
        uint32_t srcX, uint32_t srcY, uint32_t srcZ,
        uint32_t srcMipLevel,
        uint32_t srcBaseArrayLayer,
        Texture* destination,
        uint32_t dstX, uint32_t dstY, uint32_t dstZ,
        uint32_t dstMipLevel,
        uint32_t dstBaseArrayLayer,
        uint32_t width, uint32_t height, uint32_t depth,
        uint32_t layerCount);

private:
    GraphicsDevice * _gd;
    VkCommandPool _pool;
    VkCommandBuffer _cb;
    std::recursive_mutex _commandBuffersMutex;
    std::deque<VkCommandBuffer> _availableCommandBuffers;
    std::vector<VkCommandBuffer> _submittedCommandBuffers;
    uint32_t _submittedCommandBufferCount;

    bool _commandBufferBegun;
    bool _commandBufferEnded;

    bool _newFramebuffer;
    bool _currentFramebufferEverActive;
    FramebufferBase* _currentFramebuffer;
    Pipeline* _currentGraphicsPipeline;
    std::vector<ResourceSet*> _currentGraphicsResourceSets;
    std::vector<bool> _graphicsResourceSetsChanged;
    uint32_t _newGraphicsResourceSets;

    std::vector<VkRect2D> _scissorRects;
    std::vector<VkClearValue> _clearValues;
    std::optional<VkClearValue> _depthClearValue;
    std::vector<bool> _validColorClearValues;

    Pipeline* _currentComputePipeline;
    std::vector<ResourceSet*> _currentComputeResourceSets;
    std::vector<bool> _computeResourceSetsChanged;

    VkRenderPass _activeRenderPass;

    std::vector<DeviceBuffer*> _availableStagingBuffers;
    std::vector<DeviceBuffer*> _usedStagingBuffers;

    VkCommandBuffer GetNextCommandBuffer();
    void ClearCachedState();
    void BeginCurrentRenderPass();
    void EndCurrentRenderPass();
    void EnsureRenderPassActive();
    void EnsureNoRenderPass();
    void PreDrawCommand();
    DeviceBuffer* GetStagingBuffer(uint32_t size);
    void FlushNewResourceSets(
        uint32_t newResourceSetsCount,
        std::vector<ResourceSet*>& resourceSets,
        std::vector<bool>& resourceSetsChanged,
        VkPipelineBindPoint bindPoint,
        VkPipelineLayout pipelineLayout);
};
}