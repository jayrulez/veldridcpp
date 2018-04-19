#pragma once
#include "GraphicsDevice.hpp"
#include "GraphicsPipelineDescription.hpp"
#include "vulkan.h"

namespace Veldrid
{
class Pipeline
{
public:
    Pipeline(GraphicsDevice* gd, const GraphicsPipelineDescription& description);
    VkPipeline DevicePipeline;
    bool ScissorTestEnabled;
    uint32_t ResourceSetCount;
    bool IsComputePipeline;
    VkPipelineLayout PipelineLayout() const { return _pipelineLayout; }

private:
    GraphicsDevice * _gd;
    VkPipelineLayout _pipelineLayout;
    VkRenderPass _renderPass;
};
}