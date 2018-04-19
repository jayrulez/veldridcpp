#include "stdafx.h"
#include "VeldridConfig.hpp"
#include "CommandList.hpp"
#include "DeviceBuffer.hpp"
#include "ResourceFactory.hpp"
#include "Framebuffer.hpp"
#include "FormatHelpers.hpp"
#include "Util.hpp"
#include "vulkan.h"
#include <algorithm>

namespace Veldrid
{
CommandList::CommandList(GraphicsDevice* gd)
{
    _gd = gd;
    VkCommandPoolCreateInfo poolCI = {};
    poolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolCI.queueFamilyIndex = gd->GetGraphicsQueueIndex();
    CheckResult(vkCreateCommandPool(_gd->GetVkDevice(), &poolCI, nullptr, &_pool));

    _cb = GetNextCommandBuffer();
}

VkCommandBuffer CommandList::GetNextCommandBuffer()
{
    _commandBuffersMutex.lock();
    if (_availableCommandBuffers.size() > 0)
    {
        auto ret = _availableCommandBuffers.front();
        _availableCommandBuffers.pop_front();
        return ret;
    }
    _commandBuffersMutex.unlock();

    VkCommandBufferAllocateInfo cbAI;
    cbAI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbAI.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbAI.commandBufferCount = 1;
    cbAI.commandPool = _pool;
    cbAI.pNext = nullptr;

    VkCommandBuffer cb;
    CheckResult(vkAllocateCommandBuffers(_gd->GetVkDevice(), &cbAI, &cb));
    return cb;
}

VdResult CommandList::Begin()
{
    if (_commandBufferBegun)
    {
        return VdResult::InvalidOperation;
    }
    if (_commandBufferEnded)
    {
        _commandBufferEnded = false;
        _cb = GetNextCommandBuffer();
    }

    VkCommandBufferBeginInfo beginInfo;
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = nullptr;
    beginInfo.pNext = nullptr;
    CheckResult(vkBeginCommandBuffer(_cb, &beginInfo));
    _commandBufferBegun = true;

    ClearCachedState();
    _currentFramebuffer = nullptr;
    _currentGraphicsPipeline = nullptr;
    _currentGraphicsResourceSets.clear();
    _scissorRects.clear();

    _currentComputePipeline = nullptr;
    _currentComputeResourceSets.clear();

    return VdResult::Success;
}

void CommandList::ClearCachedState()
{
}

VdResult CommandList::End()
{
    if (!_commandBufferBegun)
    {
        return VdResult::InvalidOperation;
    }

    _commandBufferBegun = false;
    _commandBufferEnded = true;

    if (!_currentFramebufferEverActive && _currentFramebuffer != nullptr)
    {
        BeginCurrentRenderPass();
    }
    if (_activeRenderPass != VK_NULL_HANDLE)
    {
        EndCurrentRenderPass();
        _currentFramebuffer->TransitionToFinalLayout(_cb);
    }

    CheckResult(vkEndCommandBuffer(_cb));
    _submittedCommandBuffers.push_back(_cb);

    return VdResult::Success;
}

void CommandList::BeginCurrentRenderPass()
{
    VdAssert(_activeRenderPass == VK_NULL_HANDLE);
    VdAssert(_currentFramebuffer != nullptr);
    _currentFramebufferEverActive = true;

    uint32_t attachmentCount = _currentFramebuffer->GetAttachmentCount();
    bool haveAnyAttachments = _currentFramebuffer->ColorTargets().size() > 0 || _currentFramebuffer->DepthTarget().has_value();
    bool haveAllClearValues = _depthClearValue.has_value() || !_currentFramebuffer->DepthTarget().has_value();
    bool haveAnyClearValues = _depthClearValue.has_value();
    for (int i = 0; i < _currentFramebuffer->ColorTargets().size(); i++)
    {
        if (!_validColorClearValues[i])
        {
            haveAllClearValues = false;
            haveAnyClearValues = true;
        }
        else
        {
            haveAnyClearValues = true;
        }
    }

    VkRenderPassBeginInfo renderPassBI = {};
    renderPassBI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBI.renderArea.offset = {};
    renderPassBI.renderArea.extent.width = _currentFramebuffer->RenderableWidth();
    renderPassBI.renderArea.extent.height = _currentFramebuffer->RenderableHeight();
    renderPassBI.framebuffer = _currentFramebuffer->GetCurrentFramebuffer();

    if (!haveAnyAttachments || !haveAllClearValues)
    {
        renderPassBI.renderPass = _newFramebuffer
            ? _currentFramebuffer->GetRenderPassNoClear_Init()
            : _currentFramebuffer->GetRenderPassNoClear_Load();
        vkCmdBeginRenderPass(_cb, &renderPassBI, VK_SUBPASS_CONTENTS_INLINE);
        _activeRenderPass = renderPassBI.renderPass;

        if (haveAnyClearValues)
        {
            if (_depthClearValue.has_value())
            {
                ClearDepthStencil(_depthClearValue.value().depthStencil.depth, _depthClearValue.value().depthStencil.stencil);
                _depthClearValue.reset();
            }

            for (uint32_t i = 0; i < _currentFramebuffer->ColorTargets().size(); i++)
            {
                if (_validColorClearValues[i])
                {
                    _validColorClearValues[i] = false;
                    VkClearValue vkClearValue = _clearValues[i];
                    RgbaFloat clearColor = RgbaFloat(
                        vkClearValue.color.float32[0],
                        vkClearValue.color.float32[1],
                        vkClearValue.color.float32[2],
                        vkClearValue.color.float32[3]);
                    ClearColorTarget(i, clearColor);
                }
            }
        }
    }
    else
    {
        // We have clear values for every attachment.
        renderPassBI.renderPass = _currentFramebuffer->GetRenderPassClear();
        renderPassBI.clearValueCount = attachmentCount;
        renderPassBI.pClearValues = _clearValues.data();
        if (_depthClearValue.has_value())
        {
            _clearValues[_currentFramebuffer->GetColorAttachmentCount()] = _depthClearValue.value();
            _depthClearValue.reset();
        }
        vkCmdBeginRenderPass(_cb, &renderPassBI, VK_SUBPASS_CONTENTS_INLINE);
        _activeRenderPass = _currentFramebuffer->GetRenderPassClear();
        ClearVector(_validColorClearValues);
    }

    _newFramebuffer = false;
}

void CommandList::EndCurrentRenderPass()
{
    VdAssert(_activeRenderPass != VK_NULL_HANDLE);
    vkCmdEndRenderPass(_cb);
    _activeRenderPass = VK_NULL_HANDLE;

    // Place a barrier between RenderPasses, so that color / depth outputs
    // can be read in subsequent passes.
    vkCmdPipelineBarrier(
        _cb,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        0,
        nullptr);
}

void CommandList::EnsureRenderPassActive()
{
    if (_activeRenderPass == VK_NULL_HANDLE)
    {
        BeginCurrentRenderPass();
    }
}

void CommandList::EnsureNoRenderPass()
{
    if (_activeRenderPass != VK_NULL_HANDLE)
    {
        EndCurrentRenderPass();
    }
}

void CommandList::PreDrawCommand()
{
    EnsureRenderPassActive();

    FlushNewResourceSets(
        _newGraphicsResourceSets,
        _currentGraphicsResourceSets,
        _graphicsResourceSetsChanged,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        _currentGraphicsPipeline->PipelineLayout());
    _newGraphicsResourceSets = 0;

    if (!_currentGraphicsPipeline->ScissorTestEnabled)
    {
        SetFullScissorRects();
    }
}

VdResult CommandList::UpdateBuffer(DeviceBuffer* buffer, uint32_t offset, void* source, uint32_t size)
{
    DeviceBuffer* stagingBuffer = GetStagingBuffer(size);
    _gd->UpdateBuffer(stagingBuffer, 0, source, size);
    CopyBuffer(stagingBuffer, 0, buffer, offset, size);
    return VdResult::Success;
}

VdResult CommandList::CopyBuffer(
    DeviceBuffer* source,
    uint32_t sourceOffset,
    DeviceBuffer* destination,
    uint32_t destinationOffset,
    uint32_t size)
{
    EnsureNoRenderPass();

    VkBufferCopy region;
    region.srcOffset = sourceOffset;
    region.dstOffset = destinationOffset;
    region.size = size;

    vkCmdCopyBuffer(_cb, source->GetVkBuffer(), destination->GetVkBuffer(), 1, &region);

    return VdResult::Success;
}

VdResult CommandList::CopyTexture(
    Texture* source,
    uint32_t srcX, uint32_t srcY, uint32_t srcZ,
    uint32_t srcMipLevel, uint32_t srcBaseArrayLayer,
    Texture* destination,
    uint32_t dstX, uint32_t dstY, uint32_t dstZ,
    uint32_t dstMipLevel, uint32_t dstBaseArrayLayer,
    uint32_t width, uint32_t height, uint32_t depth,
    uint32_t layerCount)
{
    EnsureNoRenderPass();
    CopyTextureCore_CommandBuffer(
        _cb,
        source,
        srcX, srcY, srcZ,
        srcMipLevel, srcBaseArrayLayer,
        destination,
        dstX, dstY, dstZ,
        dstMipLevel, dstBaseArrayLayer,
        width, height, depth,
        layerCount);

    return VdResult::Success;
}

VdResult CommandList::SetFramebuffer(Framebuffer* fb)
{
    _newFramebuffer = true;
    if (_activeRenderPass != VK_NULL_HANDLE)
    {
        EndCurrentRenderPass();
    }
    else if (!_currentFramebufferEverActive && _currentFramebuffer != nullptr)
    {
        // This forces any queued up texture clears to be emitted.
        BeginCurrentRenderPass();
        EndCurrentRenderPass();
    }

    if (_currentFramebuffer != nullptr)
    {
        _currentFramebuffer->TransitionToFinalLayout(_cb);
    }

    _currentFramebuffer = fb;
    _currentFramebufferEverActive = false;
    EnsureMinimumSize(_scissorRects, std::max(1u, fb->GetColorAttachmentCount()));
    uint32_t clearValueCount = fb->GetColorAttachmentCount();
    EnsureMinimumSize(_clearValues, clearValueCount + 1); // Leave an extra space for the depth value (tracked separately).
    _validColorClearValues.clear();
    EnsureMinimumSize(_validColorClearValues, clearValueCount);

    return VdResult::Success;
}

VdResult CommandList::SetViewport(uint32_t index, VkViewport* viewport)
{
    vkCmdSetViewport(_cb, index, 1, viewport);
    return VdResult::Success;
}

VdResult CommandList::SetScissorRect(uint32_t index, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    VkRect2D scissor;
    scissor.offset.x = (int32_t)x;
    scissor.offset.y = (int32_t)y;
    scissor.extent.width = width;
    scissor.extent.height = height;

    if (!BlittableEqual(_scissorRects[index], scissor))
    {
        _scissorRects[index] = scissor;
        vkCmdSetScissor(_cb, index, 1, &scissor);
    }

    return VdResult::Success;
}

VdResult CommandList::ClearColorTarget(uint32_t index, RgbaFloat clearColor)
{
    VkClearValue clearValue;
    clearValue.color.float32[0] = clearColor.R;
    clearValue.color.float32[1] = clearColor.G;
    clearValue.color.float32[2] = clearColor.B;
    clearValue.color.float32[3] = clearColor.A;

    if (_activeRenderPass != VK_NULL_HANDLE)
    {
        VkClearAttachment clearAttachment;
        clearAttachment.colorAttachment = index;
        clearAttachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        clearAttachment.clearValue = clearValue;

        Texture* colorTex = _currentFramebuffer->ColorTargets()[index].Target;
        VkClearRect clearRect;
        clearRect.baseArrayLayer = 0;
        clearRect.layerCount = 1;
        clearRect.rect.offset.x = 0;
        clearRect.rect.offset.y = 0;
        clearRect.rect.extent.width = colorTex->GetWidth();
        clearRect.rect.extent.height = colorTex->GetHeight();
        vkCmdClearAttachments(_cb, 1, &clearAttachment, 1, &clearRect);
    }
    else
    {
        // Queue up the clear value for the next RenderPass.
        _clearValues[index] = clearValue;
        _validColorClearValues[index] = true;
    }

    return VdResult::Success;
}

VdResult CommandList::ClearDepthStencil(float depth, uint8_t stencil)
{
    VkClearValue clearValue;
    clearValue.depthStencil.depth = depth;
    clearValue.depthStencil.stencil = stencil;

    if (_activeRenderPass != VK_NULL_HANDLE)
    {
        Texture* depthTex = _currentFramebuffer->DepthTarget().value().Target;
        VkImageAspectFlags aspect = IsStencilFormat(depthTex->GetFormat())
            ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT
            : VK_IMAGE_ASPECT_DEPTH_BIT;
        VkClearAttachment clearAttachment = {};
        clearAttachment.aspectMask = aspect;
        clearAttachment.clearValue = clearValue;

        uint32_t renderableWidth = _currentFramebuffer->RenderableWidth();
        uint32_t renderableHeight = _currentFramebuffer->RenderableHeight();
        if (renderableWidth > 0 && renderableHeight > 0)
        {
            VkClearRect clearRect = {};
            clearRect.baseArrayLayer = 0;
            clearRect.layerCount = 1;
            clearRect.rect.extent.width = renderableWidth;
            clearRect.rect.extent.height = renderableHeight;
            vkCmdClearAttachments(_cb, 1, &clearAttachment, 1, &clearRect);
        }
    }
    else
    {
        // Queue up the clear value for the next RenderPass.
        _depthClearValue = clearValue;
    }
    return VdResult::Success;
}

VdResult CommandList::SetPipeline(Pipeline* pipeline)
{
    if (!pipeline->IsComputePipeline && _currentGraphicsPipeline != pipeline)
    {
        EnsureMinimumSize(_currentGraphicsResourceSets, pipeline->ResourceSetCount);
        ClearVector(_currentGraphicsResourceSets);
        EnsureMinimumSize(_graphicsResourceSetsChanged, pipeline->ResourceSetCount);
        vkCmdBindPipeline(_cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->DevicePipeline);
        _currentGraphicsPipeline = pipeline;
    }
    else if (pipeline->IsComputePipeline && _currentComputePipeline != pipeline)
    {
        EnsureMinimumSize(_currentComputeResourceSets, pipeline->ResourceSetCount);
        ClearVector(_currentComputeResourceSets);
        EnsureMinimumSize(_computeResourceSetsChanged, pipeline->ResourceSetCount);
        vkCmdBindPipeline(_cb, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->DevicePipeline);
        _currentComputePipeline = pipeline;
    }

    return VdResult::Success;
}

VdResult CommandList::SetVertexBuffer(uint32_t index, DeviceBuffer* buffer)
{
    VkDeviceSize offset = 0;
    VkBuffer vkBuffer = buffer->GetVkBuffer();
    vkCmdBindVertexBuffers(_cb, index, 1, &vkBuffer, &offset);

    return VdResult::Success;
}

VdResult CommandList::SetIndexBuffer(DeviceBuffer* buffer, IndexFormat format)
{
    vkCmdBindIndexBuffer(_cb, buffer->GetVkBuffer(), 0, VdToVkIndexFormat(format));

    return VdResult::Success;
}

VdResult CommandList::SetGraphicsResourceSet(uint32_t slot, ResourceSet* rs)
{
    if (_currentGraphicsResourceSets[slot] != rs)
    {
        _currentGraphicsResourceSets[slot] = rs;
        _graphicsResourceSetsChanged[slot] = true;
        _newGraphicsResourceSets += 1;
    }

    return VdResult::Success;
}

VdResult CommandList::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t vertexStart, uint32_t instanceStart)
{
    PreDrawCommand();
    vkCmdDraw(_cb, vertexCount, instanceCount, vertexStart, instanceStart);

    return VdResult::Success;
}

VdResult CommandList::DrawIndexed(
    uint32_t indexCount,
    uint32_t instanceCount,
    uint32_t indexStart,
    int32_t vertexOffset,
    uint32_t instanceStart)
{
    PreDrawCommand();
    vkCmdDrawIndexed(_cb, indexCount, instanceCount, indexStart, vertexOffset, instanceStart);

    return VdResult::Success;
}

void CommandList::SetFullScissorRects()
{
    SetScissorRect(0, 0, 0, _currentFramebuffer->Width(), _currentFramebuffer->Height());

    for (uint32_t index = 1; index < _currentFramebuffer->ColorTargets().size(); index++)
    {
        SetScissorRect(index, 0, 0, _currentFramebuffer->Width(), _currentFramebuffer->Height());
    }
}

void CommandList::CommandBufferCompleted(VkCommandBuffer completedCB)
{
    _submittedCommandBufferCount -= 1;

    _commandBuffersMutex.lock();
    for (uint32_t i = 0; i < _submittedCommandBuffers.size(); i++)
    {
        VkCommandBuffer submittedCB = _submittedCommandBuffers[i];
        if (submittedCB == completedCB)
        {
            _availableCommandBuffers.push_back(completedCB);
            _submittedCommandBuffers.erase(_submittedCommandBuffers.begin() + i);
            i -= 1;
        }
    }
    _commandBuffersMutex.unlock();
}

void CommandList::CopyTextureCore_CommandBuffer(
    VkCommandBuffer cb,
    Texture* source,
    uint32_t srcX, uint32_t srcY, uint32_t srcZ,
    uint32_t srcMipLevel, uint32_t srcBaseArrayLayer,
    Texture* destination,
    uint32_t dstX, uint32_t dstY, uint32_t dstZ,
    uint32_t dstMipLevel, uint32_t dstBaseArrayLayer,
    uint32_t width, uint32_t height, uint32_t depth,
    uint32_t layerCount)
{
    Texture* srcVkTexture = source;
    Texture* dstVkTexture = destination;

    bool sourceIsStaging = (source->GetUsage() & TextureUsage::Staging) == TextureUsage::Staging;
    bool destIsStaging = (destination->GetUsage() & TextureUsage::Staging) == TextureUsage::Staging;

    if (!sourceIsStaging && !destIsStaging)
    {
        VkImageSubresourceLayers srcSubresource;
        srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        srcSubresource.layerCount = layerCount;
        srcSubresource.mipLevel = srcMipLevel;
        srcSubresource.baseArrayLayer = srcBaseArrayLayer;

        VkImageSubresourceLayers dstSubresource;

        dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        dstSubresource.layerCount = layerCount;
        dstSubresource.mipLevel = dstMipLevel;
        dstSubresource.baseArrayLayer = dstBaseArrayLayer;

        VkImageCopy region;
        region.srcOffset.x = (int32_t)srcX;
        region.srcOffset.y = (int32_t)srcY;
        region.srcOffset.z = (int32_t)srcZ;
        region.dstOffset.x = (int32_t)dstX;
        region.dstOffset.y = (int32_t)dstY;
        region.dstOffset.z = (int32_t)dstZ;
        region.srcSubresource = srcSubresource;
        region.dstSubresource = dstSubresource;
        region.extent.width = width;
        region.extent.height = height;
        region.extent.depth = depth;

        srcVkTexture->TransitionImageLayout(
            cb,
            srcMipLevel,
            1,
            srcBaseArrayLayer,
            layerCount,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

        dstVkTexture->TransitionImageLayout(
            cb,
            dstMipLevel,
            1,
            dstBaseArrayLayer,
            layerCount,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        vkCmdCopyImage(
            cb,
            srcVkTexture->GetOptimalImage(),
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            dstVkTexture->GetOptimalImage(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region);
    }
    else if (sourceIsStaging && !destIsStaging)
    {
        VkBuffer srcBuffer = srcVkTexture->GetStagingBuffer();
        VkSubresourceLayout srcLayout = srcVkTexture->GetSubresourceLayout(
            srcVkTexture->CalculateSubresource(srcMipLevel, srcBaseArrayLayer));
        VkImage dstImage = dstVkTexture->GetOptimalImage();
        dstVkTexture->TransitionImageLayout(
            cb,
            dstMipLevel,
            1,
            dstBaseArrayLayer,
            layerCount,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkImageSubresourceLayers dstSubresource;
        dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        dstSubresource.layerCount = layerCount;
        dstSubresource.mipLevel = dstMipLevel;
        dstSubresource.baseArrayLayer = dstBaseArrayLayer;

        uint32_t mipWidth, mipHeight, mipDepth;
        GetMipDimensions(srcVkTexture, srcMipLevel, &mipWidth, &mipHeight, &mipDepth);
        uint32_t blockSize = IsCompressedFormat(srcVkTexture->GetFormat()) ? 4u : 1u;
        uint32_t bufferRowLength = std::max(mipWidth, blockSize);
        uint32_t bufferImageHeight = std::max(mipHeight, blockSize);
        uint32_t compressedX = srcX / blockSize;
        uint32_t compressedY = srcY / blockSize;
        uint32_t blockSizeInBytes = blockSize == 1
            ? GetSizeInBytes(srcVkTexture->GetFormat())
            : GetBlockSizeInBytes(srcVkTexture->GetFormat());
        uint32_t rowPitch = GetRowPitch(bufferRowLength, srcVkTexture->GetFormat());
        uint32_t depthPitch = GetDepthPitch(rowPitch, bufferImageHeight, srcVkTexture->GetFormat());

        VkBufferImageCopy region;
        region.bufferOffset = srcLayout.offset
            + (srcZ * depthPitch)
            + (compressedY * rowPitch)
            + (compressedX * blockSizeInBytes);
        region.bufferRowLength = bufferRowLength;
        region.bufferImageHeight = bufferImageHeight;
        region.imageExtent.width = width;
        region.imageExtent.height = height;
        region.imageExtent.depth = depth;
        region.imageOffset.x = (int32_t)dstX;
        region.imageOffset.y = (int32_t)dstY;
        region.imageOffset.z = (int32_t)dstZ;
        region.imageSubresource = dstSubresource;

        vkCmdCopyBufferToImage(cb, srcBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    }
    else if (!sourceIsStaging && destIsStaging)
    {
        VkImage srcImage = srcVkTexture->GetOptimalImage();
        srcVkTexture->TransitionImageLayout(
            cb,
            srcMipLevel,
            1,
            srcBaseArrayLayer,
            layerCount,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

        VkBuffer dstBuffer = dstVkTexture->GetStagingBuffer();
        VkSubresourceLayout dstLayout = dstVkTexture->GetSubresourceLayout(
            dstVkTexture->CalculateSubresource(dstMipLevel, dstBaseArrayLayer));
        VkImageSubresourceLayers srcSubresource;
        srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        srcSubresource.layerCount = layerCount;
        srcSubresource.mipLevel = srcMipLevel;
        srcSubresource.baseArrayLayer = srcBaseArrayLayer;

        uint32_t mipWidth, mipHeight, mipDepth;
        GetMipDimensions(dstVkTexture, dstMipLevel, &mipWidth, &mipHeight, &mipDepth);
        VkBufferImageCopy region;
        region.bufferRowLength = mipWidth;
        region.bufferImageHeight = mipHeight;
        region.bufferOffset = dstLayout.offset + (dstX * GetSizeInBytes(dstVkTexture->GetFormat()));
        region.imageExtent.width = width;
        region.imageExtent.height = height;
        region.imageExtent.depth = depth;
        region.imageOffset.x = (int32_t)dstX;
        region.imageOffset.y = (int32_t)dstY;
        region.imageOffset.z = (int32_t)dstZ;
        region.imageSubresource = srcSubresource;

        vkCmdCopyImageToBuffer(cb, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstBuffer, 1, &region);
    }
    else
    {
        VdAssert(sourceIsStaging && destIsStaging);
        VkBuffer srcBuffer = srcVkTexture->GetStagingBuffer();
        VkSubresourceLayout srcLayout = srcVkTexture->GetSubresourceLayout(
            srcVkTexture->CalculateSubresource(srcMipLevel, srcBaseArrayLayer));
        VkBuffer dstBuffer = dstVkTexture->GetStagingBuffer();
        VkSubresourceLayout dstLayout = dstVkTexture->GetSubresourceLayout(
            dstVkTexture->CalculateSubresource(dstMipLevel, dstBaseArrayLayer));

        uint32_t zLimit = std::max(depth, layerCount);
        if (!IsCompressedFormat(source->GetFormat()))
        {
            uint32_t pixelSize = GetSizeInBytes(srcVkTexture->GetFormat());
            for (uint32_t zz = 0; zz < zLimit; zz++)
            {
                for (uint32_t yy = 0; yy < height; yy++)
                {
                    VkBufferCopy region;
                    region.srcOffset = srcLayout.offset
                        + srcLayout.depthPitch * (zz + srcZ)
                        + srcLayout.rowPitch * (yy + srcY)
                        + pixelSize * srcX;
                    region.dstOffset = dstLayout.offset
                        + dstLayout.depthPitch * (zz + dstZ)
                        + dstLayout.rowPitch * (yy + dstY)
                        + pixelSize * dstX;
                    region.size = width * pixelSize;

                    vkCmdCopyBuffer(cb, srcBuffer, dstBuffer, 1, &region);
                }
            }
        }
        else // IsCompressedFormat
        {
            uint32_t denseRowSize = GetRowPitch(width, source->GetFormat());
            uint32_t numRows = GetNumRows(height, source->GetFormat());
            uint32_t compressedSrcX = srcX / 4;
            uint32_t compressedSrcY = srcY / 4;
            uint32_t compressedDstX = dstX / 4;
            uint32_t compressedDstY = dstY / 4;
            uint32_t blockSizeInBytes = GetBlockSizeInBytes(source->GetFormat());

            for (uint32_t zz = 0; zz < zLimit; zz++)
            {
                for (uint32_t row = 0; row < numRows; row++)
                {
                    VkBufferCopy region;
                    region.srcOffset = srcLayout.offset
                        + srcLayout.depthPitch * (zz + srcZ)
                        + srcLayout.rowPitch * (row + compressedSrcY)
                        + blockSizeInBytes * compressedSrcX;
                    region.dstOffset = dstLayout.offset
                        + dstLayout.depthPitch * (zz + dstZ)
                        + dstLayout.rowPitch * (row + compressedDstY)
                        + blockSizeInBytes * compressedDstX;
                    region.size = denseRowSize;

                    vkCmdCopyBuffer(cb, srcBuffer, dstBuffer, 1, &region);
                }
            }
        }
    }
}
DeviceBuffer* CommandList::GetStagingBuffer(uint32_t size)
{
    for (uint32_t i = 0; i < _availableStagingBuffers.size(); i++)
    {
        DeviceBuffer* buffer = _availableStagingBuffers[i];
        if (buffer->GetSizeInBytes() >= size)
        {
            _availableStagingBuffers.erase(_availableStagingBuffers.begin() + i);
            _usedStagingBuffers.push_back(buffer);
            return buffer;
        }
    }

    DeviceBuffer* newBuffer = _gd->GetResourceFactory()->CreateBuffer(BufferDescription(size, BufferUsage::Staging));
    _usedStagingBuffers.push_back(newBuffer);
    return newBuffer;
}

void CommandList::FlushNewResourceSets(
    uint32_t newResourceSetsCount,
    std::vector<ResourceSet*>& resourceSets,
    std::vector<bool>& resourceSetsChanged,
    VkPipelineBindPoint bindPoint,
    VkPipelineLayout pipelineLayout)
{
    if (newResourceSetsCount > 0)
    {
        uint32_t totalChanged = 0;
        uint32_t currentSlot = 0;
        uint32_t currentBatchIndex = 0;
        uint32_t currentBatchFirstSet = 0;
        std::vector<VkDescriptorSet> descriptorSets(newResourceSetsCount);
        while (totalChanged < newResourceSetsCount)
        {
            if (resourceSetsChanged[currentSlot])
            {
                resourceSetsChanged[currentSlot] = false;
                descriptorSets[currentBatchIndex] = resourceSets[currentSlot]->DescriptorSet();
                totalChanged += 1;
                currentBatchIndex += 1;
                currentSlot += 1;
            }
            else
            {
                if (currentBatchIndex != 0)
                {
                    // Flush current batch.
                    vkCmdBindDescriptorSets(
                        _cb,
                        bindPoint,
                        pipelineLayout,
                        currentBatchFirstSet,
                        currentBatchIndex,
                        descriptorSets.data(),
                        0,
                        nullptr);
                    currentBatchIndex = 0;
                }

                currentSlot += 1;
                currentBatchFirstSet = currentSlot;
            }
        }

        if (currentBatchIndex != 0)
        {
            // Flush current batch.
            vkCmdBindDescriptorSets(
                _cb,
                bindPoint,
                pipelineLayout,
                currentBatchFirstSet,
                currentBatchIndex,
                descriptorSets.data(),
                0,
                nullptr);
        }
    }
}

VdResult CommandList::Dispose()
{
    _gd->EnqueueDisposedCommandBuffer(this);
    return VdResult::Success;
}

VD_EXPORT VdResult VdCommandList_Begin(CommandList* cl) { return cl->Begin(); }
VD_EXPORT VdResult VdCommandList_End(CommandList* cl) { return cl->End(); }
VD_EXPORT VdResult VdCommandList_Dispose(CommandList* cl) { return cl->Dispose(); }
VD_EXPORT VdResult VdCommandList_UpdateBuffer(
    CommandList* cl,
    DeviceBuffer* buffer,
    uint32_t offset,
    void* source,
    uint32_t size)
{
    return cl->UpdateBuffer(buffer, offset, source, size);
}

VD_EXPORT VdResult VdCommandList_CopyBuffer(
    CommandList* cl,
    DeviceBuffer* source,
    uint32_t sourceOffset,
    DeviceBuffer* destination,
    uint32_t destinationOffset,
    uint32_t size)
{
    return cl->CopyBuffer(source, sourceOffset, destination, destinationOffset, size);
}

VD_EXPORT VdResult VdCommandList_CopyTexture(
    CommandList* cl,
    Texture* source,
    uint32_t srcX, uint32_t srcY, uint32_t srcZ,
    uint32_t srcMipLevel, uint32_t srcBaseArrayLayer,
    Texture* destination,
    uint32_t dstX, uint32_t dstY, uint32_t dstZ,
    uint32_t dstMipLevel, uint32_t dstBaseArrayLayer,
    uint32_t width, uint32_t height, uint32_t depth,
    uint32_t layerCount)
{
    return cl->CopyTexture(
        source,
        srcX, srcY, srcZ,
        srcMipLevel, srcBaseArrayLayer,
        destination,
        dstX, dstY, dstZ,
        dstMipLevel, dstBaseArrayLayer,
        width, height, depth,
        layerCount);
}

VD_EXPORT VdResult VdCommandList_SetFramebuffer(
    CommandList* cl,
    Framebuffer* fb)
{
    return cl->SetFramebuffer(fb);
}

VD_EXPORT VdResult VdCommandList_SetViewport(
    CommandList* cl,
    uint32_t index,
    VkViewport* viewport)
{
    return cl->SetViewport(index, viewport);
}

VD_EXPORT VdResult VdCommandList_SetScissorRect(
    CommandList* cl,
    uint32_t index,
    uint32_t x,
    uint32_t y,
    uint32_t width,
    uint32_t height)
{
    return cl->SetScissorRect(index, x, y, width, height);
}

VD_EXPORT VdResult VdCommandList_ClearColorTarget(
    CommandList* cl,
    uint32_t index,
    RgbaFloat color)
{
    return cl->ClearColorTarget(index, color);
}

VD_EXPORT VdResult VdCommandList_ClearDepthStencil(
    CommandList* cl,
    float depth,
    uint8_t stencil)
{
    return cl->ClearDepthStencil(depth, stencil);
}

VD_EXPORT VdResult VdCommandList_SetPipeline(CommandList* cl, Pipeline* pipeline)
{
    return cl->SetPipeline(pipeline);
}

VD_EXPORT VdResult VdCommandList_SetVertexBuffer(CommandList* cl, uint32_t index, DeviceBuffer* buffer)
{
    return cl->SetVertexBuffer(index, buffer);
}

VD_EXPORT VdResult VdCommandList_SetIndexBuffer(CommandList* cl, DeviceBuffer* buffer, IndexFormat format)
{
    return cl->SetIndexBuffer(buffer, format);
}

VD_EXPORT VdResult VdCommandList_SetGraphicsResourceSet(CommandList* cl, uint32_t slot, ResourceSet* rs)
{
    return cl->SetGraphicsResourceSet(slot, rs);
}

VD_EXPORT VdResult VdCommandList_Draw(
    CommandList* cl,
    uint32_t vertexCount,
    uint32_t instanceCount,
    uint32_t vertexStart,
    uint32_t instanceStart)
{
    return cl->Draw(vertexCount, instanceCount, vertexStart, instanceStart);
}

VD_EXPORT VdResult VdCommandList_DrawIndexed(
    CommandList* cl,
    uint32_t indexCount,
    uint32_t instanceCount,
    uint32_t indexStart,
    int32_t vertexOffset,
    uint32_t instanceStart)
{
    return cl->DrawIndexed(indexCount, instanceCount, indexStart, vertexOffset, instanceStart);
}
}
