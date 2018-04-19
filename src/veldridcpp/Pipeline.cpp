#include "stdafx.h"
#include "Pipeline.hpp"
#include "FormatHelpers.hpp"
#include "VkFormats.hpp"
#include <array>

namespace Veldrid
{
Pipeline::Pipeline(GraphicsDevice* gd, const GraphicsPipelineDescription& description)
{
    _gd = gd;

    VkGraphicsPipelineCreateInfo pipelineCI = {};
    pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    // Blend State
    VkPipelineColorBlendStateCreateInfo blendStateCI = {};
    blendStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    uint32_t attachmentsCount = description.BlendState.AttachmentStates.Count;
    std::vector<VkPipelineColorBlendAttachmentState> attachments(attachmentsCount);
    for (uint32_t i = 0; i < attachmentsCount; i++)
    {
        const BlendAttachmentDescription& vdDesc = description.BlendState.AttachmentStates[i];
        VkPipelineColorBlendAttachmentState attachmentState = {};
        attachmentState.srcColorBlendFactor = VdToVkBlendFactor(vdDesc.SourceColorFactor);
        attachmentState.dstColorBlendFactor = VdToVkBlendFactor(vdDesc.DestinationColorFactor);
        attachmentState.colorBlendOp = VdToVkBlendOp(vdDesc.ColorFunction);
        attachmentState.srcAlphaBlendFactor = VdToVkBlendFactor(vdDesc.SourceAlphaFactor);
        attachmentState.dstAlphaBlendFactor = VdToVkBlendFactor(vdDesc.DestinationAlphaFactor);
        attachmentState.alphaBlendOp = VdToVkBlendOp(vdDesc.AlphaFunction);
        attachmentState.blendEnable = vdDesc.BlendEnabled;
        attachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        attachments[i] = attachmentState;
    }

    blendStateCI.attachmentCount = attachmentsCount;
    blendStateCI.pAttachments = attachments.data();
    RgbaFloat blendFactor = description.BlendState.BlendFactor;
    blendStateCI.blendConstants[0] = blendFactor.R;
    blendStateCI.blendConstants[1] = blendFactor.G;
    blendStateCI.blendConstants[2] = blendFactor.B;
    blendStateCI.blendConstants[3] = blendFactor.A;

    pipelineCI.pColorBlendState = &blendStateCI;
    // Rasterizer State
    RasterizerStateDescription rsDesc = description.RasterizerState;
    VkPipelineRasterizationStateCreateInfo rsCI = {};
    rsCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rsCI.cullMode = VdToVkCullMode(rsDesc.CullMode);
    rsCI.polygonMode = VdToVkPolygonMode(rsDesc.FillMode);
    rsCI.depthClampEnable = !rsDesc.DepthClipEnabled;
    rsCI.frontFace = rsDesc.FrontFace == FrontFace::Clockwise ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rsCI.lineWidth = 1.0f;

    pipelineCI.pRasterizationState = &rsCI;

    ScissorTestEnabled = rsDesc.ScissorTestEnabled;

    // Dynamic State
    VkPipelineDynamicStateCreateInfo dynamicStateCI = {};
    dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    std::array<VkDynamicState, 2> dynamicStates =
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    dynamicStateCI.dynamicStateCount = 2;
    dynamicStateCI.pDynamicStates = dynamicStates.data();

    pipelineCI.pDynamicState = &dynamicStateCI;

    // Depth Stencil State
    DepthStencilStateDescription vdDssDesc = description.DepthStencilState;
    VkPipelineDepthStencilStateCreateInfo dssCI = {};
    dssCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    dssCI.depthWriteEnable = vdDssDesc.DepthWriteEnabled;
    dssCI.depthTestEnable = vdDssDesc.DepthTestEnabled;
    dssCI.depthCompareOp = VdToVkCompareOp(vdDssDesc.DepthComparison);
    dssCI.stencilTestEnable = vdDssDesc.StencilTestEnabled;

    dssCI.front.failOp = VdToVkStencilOp(vdDssDesc.StencilFront.Fail);
    dssCI.front.passOp = VdToVkStencilOp(vdDssDesc.StencilFront.Pass);
    dssCI.front.depthFailOp = VdToVkStencilOp(vdDssDesc.StencilFront.DepthFail);
    dssCI.front.compareMask = vdDssDesc.StencilReadMask;
    dssCI.front.writeMask = vdDssDesc.StencilWriteMask;
    dssCI.front.reference = vdDssDesc.StencilReference;

    dssCI.back.failOp = VdToVkStencilOp(vdDssDesc.StencilBack.Fail);
    dssCI.back.passOp = VdToVkStencilOp(vdDssDesc.StencilBack.Pass);
    dssCI.back.depthFailOp = VdToVkStencilOp(vdDssDesc.StencilBack.DepthFail);
    dssCI.back.compareMask = vdDssDesc.StencilReadMask;
    dssCI.back.writeMask = vdDssDesc.StencilWriteMask;
    dssCI.back.reference = vdDssDesc.StencilReference;

    pipelineCI.pDepthStencilState = &dssCI;

    // Multisample
    VkPipelineMultisampleStateCreateInfo multisampleCI = {};
    multisampleCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    VkSampleCountFlags vkSampleCount = VdToVkSampleCount(description.Outputs.SampleCount);
    multisampleCI.rasterizationSamples = (VkSampleCountFlagBits)vkSampleCount;

    pipelineCI.pMultisampleState = &multisampleCI;

    // Input Assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCI = {};
    inputAssemblyCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyCI.topology = VdToVkPrimitiveTopology(description.PrimitiveTopology);

    pipelineCI.pInputAssemblyState = &inputAssemblyCI;

    // Vertex Input State
    VkPipelineVertexInputStateCreateInfo vertexInputCI = {};
    vertexInputCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    const auto& inputDescriptions = description.ShaderSet.VertexLayouts;
    uint32_t bindingCount = inputDescriptions.Count;
    uint32_t attributeCount = 0;
    for (uint32_t i = 0; i < bindingCount; i++)
    {
        attributeCount += inputDescriptions[i].Elements.Count;
    }
    std::vector<VkVertexInputBindingDescription> bindingDescs(bindingCount);
    std::vector<VkVertexInputAttributeDescription> attributeDescs(attributeCount);

    uint32_t targetIndex = 0;
    uint32_t targetLocation = 0;
    for (uint32_t binding = 0; binding < bindingCount; binding++)
    {
        const VertexLayoutDescription& inputDesc = inputDescriptions[binding];
        bindingDescs[binding].binding = binding;
        bindingDescs[binding].inputRate = (inputDesc.InstanceStepRate != 0) ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
        bindingDescs[binding].stride = inputDesc.Stride;

        uint32_t currentOffset = 0;
        for (uint32_t location = 0; location < inputDesc.Elements.Count; location++)
        {
            const VertexElementDescription& inputElement = inputDesc.Elements[location];

            attributeDescs[targetIndex].format = VdToVkVertexElementFormat(inputElement.Format);
            attributeDescs[targetIndex].binding = binding;
            attributeDescs[targetIndex].location = targetLocation + location;
            attributeDescs[targetIndex].offset = currentOffset;

            targetIndex += 1;
            currentOffset += GetSizeInBytes(inputElement.Format);
        }

        targetLocation += inputDesc.Elements.Count;
    }

    vertexInputCI.vertexBindingDescriptionCount = bindingCount;
    vertexInputCI.pVertexBindingDescriptions = bindingDescs.data();
    vertexInputCI.vertexAttributeDescriptionCount = attributeCount;
    vertexInputCI.pVertexAttributeDescriptions = attributeDescs.data();

    pipelineCI.pVertexInputState = &vertexInputCI;

    // Shader Stage
    const auto& shaders = description.ShaderSet.Shaders;
    std::vector<VkPipelineShaderStageCreateInfo> stages;
    for (uint32_t i = 0; i < shaders.Count; i++)
    {
        Shader* shader = shaders[i];
        VkPipelineShaderStageCreateInfo stageCI = {};
        stageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stageCI.module = shader->ShaderModule();
        stageCI.stage = (VkShaderStageFlagBits)VdToVkShaderStages(shader->Stage());
        stageCI.pName = "main";
        stages.push_back(stageCI);
    }

    pipelineCI.stageCount = shaders.Count;
    pipelineCI.pStages = stages.data();

    // ViewportState
    VkPipelineViewportStateCreateInfo viewportStateCI = {};
    viewportStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCI.viewportCount = 1;
    viewportStateCI.scissorCount = 1;

    pipelineCI.pViewportState = &viewportStateCI;

    // Pipeline Layout
    const auto& resourceLayouts = description.ResourceLayouts;
    VkPipelineLayoutCreateInfo pipelineLayoutCI = {};
    pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    std::vector<VkDescriptorSetLayout> dsls(resourceLayouts.Count);
    for (uint32_t i = 0; i < resourceLayouts.Count; i++)
    {
        dsls[i] = resourceLayouts[i]->DescriptorSetLayout();
    }

    pipelineLayoutCI.setLayoutCount = resourceLayouts.Count;
    pipelineLayoutCI.pSetLayouts = dsls.data();

    vkCreatePipelineLayout(_gd->GetVkDevice(), &pipelineLayoutCI, nullptr, &_pipelineLayout);
    pipelineCI.layout = _pipelineLayout;

    // Create fake RenderPass for compatibility.

    VkRenderPassCreateInfo renderPassCI = {};
    renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    const auto& outputDesc = description.Outputs;
    std::vector<VkAttachmentDescription> attachmentDescs;

    std::vector<VkAttachmentDescription> colorAttachmentDescs(outputDesc.ColorAttachments.Count);
    std::vector<VkAttachmentReference> colorAttachmentRefs(outputDesc.ColorAttachments.Count);
    for (uint32_t i = 0; i < outputDesc.ColorAttachments.Count; i++)
    {
        colorAttachmentDescs[i].format = VdToVkPixelFormat(outputDesc.ColorAttachments[i]);
        colorAttachmentDescs[i].samples = (VkSampleCountFlagBits)vkSampleCount;
        colorAttachmentDescs[i].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentDescs[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentDescs[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentDescs[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentDescs[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentDescs[i].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        attachmentDescs.push_back(colorAttachmentDescs[i]);

        colorAttachmentRefs[i].attachment = i;
        colorAttachmentRefs[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    VkAttachmentDescription depthAttachmentDesc = {};
    VkAttachmentReference depthAttachmentRef = {};
    if (outputDesc.DepthAttachment != nullptr)
    {
        PixelFormat depthFormat = *outputDesc.DepthAttachment;
        bool hasStencil = IsStencilFormat(depthFormat);
        depthAttachmentDesc.format = VdToVkPixelFormat(depthFormat, true);
        depthAttachmentDesc.samples = (VkSampleCountFlagBits)vkSampleCount;
        depthAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachmentDesc.stencilStoreOp = hasStencil ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        depthAttachmentRef.attachment = outputDesc.ColorAttachments.Count;
        depthAttachmentRef.layout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = outputDesc.ColorAttachments.Count;
    subpass.pColorAttachments = colorAttachmentRefs.data();
    for (uint32_t i = 0; i < colorAttachmentDescs.size(); i++)
    {
        attachmentDescs.push_back(colorAttachmentDescs[i]);
    }

    if (outputDesc.DepthAttachment != nullptr)
    {
        subpass.pDepthStencilAttachment = &depthAttachmentRef;
        attachmentDescs.push_back(depthAttachmentDesc);
    }

    VkSubpassDependency subpassDependency = {};
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    if (outputDesc.DepthAttachment != nullptr)
    {
        subpassDependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }

    renderPassCI.attachmentCount = static_cast<uint32_t>(attachmentDescs.size());
    renderPassCI.pAttachments = (VkAttachmentDescription*)attachmentDescs.data();
    renderPassCI.subpassCount = 1;
    renderPassCI.pSubpasses = &subpass;
    renderPassCI.dependencyCount = 1;
    renderPassCI.pDependencies = &subpassDependency;

    VkResult creationResult = vkCreateRenderPass(_gd->GetVkDevice(), &renderPassCI, nullptr, &_renderPass);
    CheckResult(creationResult);

    pipelineCI.renderPass = _renderPass;

    VkResult result = vkCreateGraphicsPipelines(_gd->GetVkDevice(), VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &DevicePipeline);
    CheckResult(result);

    ResourceSetCount = description.ResourceLayouts.Count;
}

VD_EXPORT void VdPipeline_Dispose(Pipeline* pipeline)
{
    delete pipeline;
}
}
