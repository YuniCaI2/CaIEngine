//
// Created by 51092 on 25-5-10.
//

#include "VulkanUIOverlay.h"

#include <iostream>

#include "VulkanTool.h"

FrameWork::VulkanUIOverlay::VulkanUIOverlay() {
    ImGui::CreateContext();
    ImGuiStyle &style = ImGui::GetStyle();
    style.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.0f, 0.0f, 0.0f, 0.1f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.8f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.8f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.1f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(1.0f, 1.0f, 1.0f, 0.2f);
    style.Colors[ImGuiCol_Button] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
    //控制全局的字体缩放
    ImGuiIO &io = ImGui::GetIO();
    io.FontGlobalScale = scale;
}


FrameWork::VulkanUIOverlay::~VulkanUIOverlay() {
    if (ImGui::GetCurrentContext()) {
        ImGui::DestroyContext();
    }
}

void FrameWork::VulkanUIOverlay::preparePipeline(const VkPipelineCache &pipelineCache, const VkRenderPass &renderPass,
                                                 const VkFormat &colorFormat, const VkFormat &depthFormat) {
    //Pipeline layout
    //Push Constants
    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstBlock);
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    VK_CHECK_RESULT(vkCreatePipelineLayout(device->logicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout));

    //设置图像管线

    //图元装配
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
    inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyState.primitiveRestartEnable = VK_FALSE; //关闭图元重启，图元重启是相当于可以复用三角形顶点，以此来减少顶点数

    //光栅设置（面剔除、填充）
    VkPipelineRasterizationStateCreateInfo rasterizationState = {};
    rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationState.depthClampEnable = VK_FALSE;
    rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationState.lineWidth = 1.0f;
    rasterizationState.cullMode = VK_CULL_MODE_NONE;
    rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    //blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
                                          | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlendState = {};
    colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendState.attachmentCount = 1;
    colorBlendState.pAttachments = &colorBlendAttachment;

    VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
    depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilState.depthTestEnable = VK_FALSE;
    depthStencilState.depthWriteEnable = VK_FALSE;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_ALWAYS;
    depthStencilState.depthBoundsTestEnable = VK_FALSE;
    depthStencilState.stencilTestEnable = VK_FALSE;
    depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;
    viewportState.flags = 0;

    VkPipelineMultisampleStateCreateInfo multisampleState = {};
    multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleState.rasterizationSamples = rasterizationSamples;
    multisampleState.flags = 0;

    std::vector<VkDynamicState> dynamicStateEnables = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
    dynamicState.pDynamicStates = dynamicStateEnables.data();

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.layout = pipelineLayout;
    pipelineCreateInfo.renderPass = renderPass;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
    pipelineCreateInfo.pViewportState = &viewportState;
    pipelineCreateInfo.pRasterizationState = &rasterizationState;
    pipelineCreateInfo.pColorBlendState = &colorBlendState;
    pipelineCreateInfo.pMultisampleState = &multisampleState;
    pipelineCreateInfo.pDepthStencilState = &depthStencilState;
    pipelineCreateInfo.pDynamicState = &dynamicState;
    pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaders.size());
    pipelineCreateInfo.pStages = shaders.data();
    pipelineCreateInfo.subpass = subpass;

#if defined(VK_KHR_dynamic_rendering)// 向下兼容VK 1.2
    VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo = {};
    //动态渲染不需要renderpass
    if (renderPass == VK_NULL_HANDLE) {
        pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        pipelineRenderingCreateInfo.colorAttachmentCount = 1;
        pipelineRenderingCreateInfo.pColorAttachmentFormats = &colorFormat;
        pipelineRenderingCreateInfo.depthAttachmentFormat = depthFormat;
        pipelineRenderingCreateInfo.stencilAttachmentFormat = depthFormat;
        pipelineCreateInfo.pNext = &pipelineRenderingCreateInfo;
    }
#endif

    //Vertex bindings an attributes based on ImGui vertex difinition
    std::vector<VkVertexInputBindingDescription> vertexInputBindings = {};
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(ImDrawVert);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    vertexInputBindings.push_back(bindingDescription);

    std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {};
    VkVertexInputAttributeDescription attributeDescription = {};
    attributeDescription.location = 0;
    attributeDescription.binding = 0;
    attributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescription.offset = offsetof(ImDrawVert, pos);
    vertexInputAttributes.push_back(attributeDescription);

    attributeDescription.location = 1;
    attributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescription.offset = offsetof(ImDrawVert, uv);
    vertexInputAttributes.push_back(attributeDescription);

    attributeDescription.location = 2;
    attributeDescription.format = VK_FORMAT_R8G8B8A8_UNORM;
    attributeDescription.offset = offsetof(ImDrawVert, col);
    vertexInputAttributes.push_back(attributeDescription);

    VkPipelineVertexInputStateCreateInfo vertexInputState = {};
    vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
    vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
    vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
    vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();


    pipelineCreateInfo.pVertexInputState = &vertexInputState;

    VK_CHECK_RESULT(
        vkCreateGraphicsPipelines(device->logicalDevice, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline));
}

//为ImGui准备Vulkan资源
void FrameWork::VulkanUIOverlay::prepareResources() {
    ImGuiIO &io = ImGui::GetIO();

    //Create font texture
    unsigned char *fontData;
    int texWidth, texHeight;

    const std::string filename = VulkanTool::getAssetPath() + "font/SIMHEI.TTF";
    io.Fonts->AddFontFromFileTTF(filename.c_str(), 16.0f * scale);
    io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
    VkDeviceSize uploadSize = texWidth * texHeight * 4 * sizeof(char);

    ImGuiStyle &style = ImGui::GetStyle();
    style.ScaleAllSizes(scale);

    //Create target image for copy
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.extent.width = texWidth;
    imageInfo.extent.height = texHeight;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VK_CHECK_RESULT(vkCreateImage(device->logicalDevice, &imageInfo, nullptr, &fontImage));
    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(device->logicalDevice, fontImage, &memReqs);
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_CHECK_RESULT(vkAllocateMemory(device->logicalDevice, &allocInfo, nullptr, &fontMemory));
    VK_CHECK_RESULT(vkBindImageMemory(device->logicalDevice, fontImage, fontMemory, 0));

    //Image view
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = fontImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    VK_CHECK_RESULT(vkCreateImageView(device->logicalDevice, &viewInfo, nullptr, &fontView));

    //Stage buffers for font data upload
    Buffer stagingBuffer;

    VK_CHECK_RESULT(device->createBuffer(
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &stagingBuffer,
        uploadSize
    ));

    stagingBuffer.map();
    memcpy(stagingBuffer.mapped, fontData, uploadSize);
    stagingBuffer.unmap();

    //CopyBuffer data to font image
    VkCommandBuffer copyCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

    VulkanTool::transitionImageLayout(
        copyCmd,
        fontImage,
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_HOST_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT
    );

    VkBufferImageCopy region = {};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent.width = texWidth;
    region.imageExtent.height = texHeight;
    region.imageExtent.depth = 1;

    vkCmdCopyBufferToImage(
        copyCmd,
        stagingBuffer.buffer,
        fontImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );

    //Prepare for shader read
    VulkanTool::transitionImageLayout(
        copyCmd,
        fontImage,
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
    );

    //提交
    device->flushCommandBuffer(copyCmd, queue, true);

    stagingBuffer.destroy();

    //字体的采样器
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;

    VK_CHECK_RESULT(vkCreateSampler(device->logicalDevice, &samplerInfo, nullptr, &sampler));

    //Descriptor pool
    std::vector<VkDescriptorPoolSize> poolSizes = {};
    VkDescriptorPoolSize poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 1;
    poolSizes.push_back(poolSize);
    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 2;
    VK_CHECK_RESULT(vkCreateDescriptorPool(device->logicalDevice, &poolInfo, nullptr, &descriptorPool));

    //Descriptor set layout
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {};
    VkDescriptorSetLayoutBinding layoutBinding = {};
    layoutBinding.binding = 0;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    //这里的数量指的是绑定点上绑几个对象
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    setLayoutBindings.push_back(layoutBinding);

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
    layoutInfo.pBindings = setLayoutBindings.data();
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device->logicalDevice, &layoutInfo, nullptr, &descriptorSetLayout));
    //布局实在分配描述符集的时候使用的

    VkDescriptorSetAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = descriptorPool;
    allocateInfo.descriptorSetCount = 1; //分配描述符集的数量
    allocateInfo.pSetLayouts = &descriptorSetLayout;
    VK_CHECK_RESULT(vkAllocateDescriptorSets(device->logicalDevice, &allocateInfo, &descriptorSet));
    VkDescriptorImageInfo fontDescriptor = {};
    fontDescriptor.sampler = sampler;
    fontDescriptor.imageView = fontView;
    fontDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkWriteDescriptorSet writeDescriptor = {};
    writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptor.dstSet = descriptorSet;
    writeDescriptor.dstBinding = 0;
    writeDescriptor.dstArrayElement = 0; //数组起始的元素
    writeDescriptor.descriptorCount = 1; //更新的元素
    writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeDescriptor.pImageInfo = &fontDescriptor;
    vkUpdateDescriptorSets(device->logicalDevice, 1, &writeDescriptor, 0, nullptr);
}

bool FrameWork::VulkanUIOverlay::update() {
    ImDrawData *imDrawData = ImGui::GetDrawData();
    bool updateCmdBuffers = false;

    if (!imDrawData) { return false; }

    VkDeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
    VkDeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

    if ((vertexBufferSize == 0) || (indexBufferSize == 0)) { return false; }

    //Vertex buffer 更新Vertex Buffer
    if ((vertexBuffer.buffer == VK_NULL_HANDLE) || (vertexCount != imDrawData->TotalVtxCount)) {
        vertexBuffer.unmap();
        vertexBuffer.destroy();
        VK_CHECK_RESULT(
            device->createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &vertexBuffer, vertexBufferSize)
        ); // 主机可见但是没同步
        vertexCount = imDrawData->TotalVtxCount;
        vertexBuffer.unmap();
        vertexBuffer.map();
        updateCmdBuffers = true;
    }

    //Index buffer
    if ((indexBuffer.buffer == VK_NULL_HANDLE) || (indexCount != imDrawData->TotalIdxCount)) {
        indexBuffer.unmap();
        indexBuffer.destroy();
        VK_CHECK_RESULT(device->createBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &indexBuffer, indexBufferSize));
        indexCount = imDrawData->TotalIdxCount;
        indexBuffer.unmap();
        indexBuffer.map();
        updateCmdBuffers = true;
    }

    //Upload data
    auto vtxDst = static_cast<ImDrawVert *>(vertexBuffer.mapped);
    auto idxDst = static_cast<ImDrawIdx *>(indexBuffer.mapped);

    for (int n = 0; n < imDrawData->CmdListsCount; n++) {
        const ImDrawList *cmd_list = imDrawData->CmdLists[n];
        memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        vtxDst += cmd_list->VtxBuffer.Size; //控制指针偏移
        idxDst += cmd_list->IdxBuffer.Size; //控制指针偏移
    }

    //使得GPU可见
    vertexBuffer.flush();
    indexBuffer.flush();

    return updateCmdBuffers;
}

void FrameWork::VulkanUIOverlay::draw(const VkCommandBuffer &commandBuffer) {
    ImDrawData *imDrawData = ImGui::GetDrawData();
    int32_t vertexOffset = 0;
    int32_t indexOffset = 0;

    if ((!imDrawData) || (imDrawData->CmdListsCount == 0)) { return; }

    ImGuiIO &io = ImGui::GetIO();

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0,
                            nullptr);

    VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer, offsets);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16); //和Imgui对齐

    for (int32_t i = 0; i < imDrawData->CmdListsCount; i++) {
        const ImDrawList *cmd_list = imDrawData->CmdLists[i];
        for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++) {
            const ImDrawCmd *pcmd = &cmd_list->CmdBuffer[j];
            VkRect2D scissorRect; //将List内容转移到commandbuffer
            scissorRect.offset.x = std::max((int32_t) (pcmd->ClipRect.x), 0);
            scissorRect.offset.y = std::max((int32_t) (pcmd->ClipRect.y), 0);
            scissorRect.extent.width = (uint32_t) (pcmd->ClipRect.z - pcmd->ClipRect.x);
            scissorRect.extent.height = (uint32_t) (pcmd->ClipRect.w - pcmd->ClipRect.y);
            vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);
            vkCmdDrawIndexed(commandBuffer, pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
            indexOffset += pcmd->ElemCount;
        }
        vertexOffset += cmd_list->VtxBuffer.Size; //一个lists实现一次偏移
    }
}

void FrameWork::VulkanUIOverlay::resize(uint32_t width, uint32_t height) {
    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float) (width), (float) (height));
}

void FrameWork::VulkanUIOverlay::freeResources() {
    vertexBuffer.destroy();
    indexBuffer.destroy();
    vkDestroyImageView(device->logicalDevice, fontView, nullptr);
    vkDestroyImage(device->logicalDevice, fontImage, nullptr);
    vkFreeMemory(device->logicalDevice, fontMemory, nullptr);
    vkDestroySampler(device->logicalDevice, sampler, nullptr);
    vkDestroyDescriptorSetLayout(device->logicalDevice, descriptorSetLayout, nullptr);
    vkDestroyDescriptorPool(device->logicalDevice, descriptorPool, nullptr);
    vkDestroyPipelineLayout(device->logicalDevice, pipelineLayout, nullptr);
    vkDestroyPipeline(device->logicalDevice, pipeline, nullptr);
}

bool FrameWork::VulkanUIOverlay::header(const char *caption) {
    return ImGui::CollapsingHeader(caption, ImGuiTreeNodeFlags_DefaultOpen);
    //首次展示的时候展开
}

bool FrameWork::VulkanUIOverlay::checkBox(const char *caption, bool *value) {
    bool res = ImGui::Checkbox(caption, value);
    if (res) { updated = true; };
    return res;
}

bool FrameWork::VulkanUIOverlay::checkBox(const char *caption, int32_t *value) {
    bool val = (*value == 1);
    bool res = ImGui::Checkbox(caption, &val);
    *value = val;
    if (res) { updated = true; };
    return res;
}

bool FrameWork::VulkanUIOverlay::radioButton(const char *caption, bool value) {
    bool res = ImGui::RadioButton(caption, value);
    if (res) { updated = true; };
    return res;
}

bool FrameWork::VulkanUIOverlay::inputFloat(const char *caption, float *value, float step, uint32_t precision) {
    bool res = ImGui::InputFloat(caption, value, step, step * 10.0f, precision);
    if (res) { updated = true; };
    return res;
}

bool FrameWork::VulkanUIOverlay::sliderFloat(const char *caption, float *value, float min, float max) {
    bool res = ImGui::SliderFloat(caption, value, min, max);
    if (res) { updated = true; };
    return res;
}

bool FrameWork::VulkanUIOverlay::sliderInt(const char *caption, int32_t *value, int32_t min, int32_t max) {
    bool res = ImGui::SliderInt(caption, value, min, max);
    if (res) { updated = true; };
    return res;
}

bool FrameWork::VulkanUIOverlay::comboBox(const char *caption, int32_t *itemindex, std::vector<std::string> items) {
    if (items.empty()) {
        return false;
    }
    std::vector<const char *> charitems;
    charitems.reserve(items.size());
    for (size_t i = 0; i < items.size(); i++) {
        charitems.push_back(items[i].c_str());
    }
    uint32_t itemCount = static_cast<uint32_t>(charitems.size());
    bool res = ImGui::Combo(caption, itemindex, &charitems[0], itemCount, itemCount);
    if (res) { updated = true; };
    return res;
}

bool FrameWork::VulkanUIOverlay::button(const char *caption) {
    bool res = ImGui::Button(caption);
    if (res) { updated = true; };
    return res;
}

bool FrameWork::VulkanUIOverlay::colorPicker(const char *caption, float *color) {
    bool res = ImGui::ColorEdit4(caption, color, ImGuiColorEditFlags_NoInputs);
    if (res) { updated = true; };
    return res;
}

void FrameWork::VulkanUIOverlay::text(const char *formatstr, ...) {
    va_list args;
    va_start(args, formatstr);
    ImGui::TextV(formatstr, args);
    va_end(args);
}
