//
// Created by cai on 2025/9/13.
//

#include "FrameGraph.h"

#include <unordered_set>

#include "ResourceManager.h"

FG::FrameGraph::FrameGraph(ResourceManager &resourceManager,
                           RenderPassManager &renderPassManager) : resourceManager(resourceManager),
                                                                   renderPassManager(renderPassManager), threadPool(8) {
}

FG::FrameGraph::~FrameGraph() {
    // 清理command pools
    for (auto &[passIndex, pool]: renderPassCommandPools) {
        if (pool != VK_NULL_HANDLE)
            vkDestroyCommandPool(vulkanRenderAPI.GetVulkanDevice()->logicalDevice,
                                 pool, nullptr);
        pool = VK_NULL_HANDLE;
    }
    renderPassCommandPools.clear();
}

FG::FrameGraph &FG::FrameGraph::AddResourceNode(uint32_t resourceNode) {
    //防止重复
    if (std::find(resourceNodes.begin(), resourceNodes.end(), resourceNode)
        == resourceNodes.end()) {
        resourceNodes.push_back(resourceNode);
    }
    return *this;
}

FG::FrameGraph &FG::FrameGraph::AddRenderPassNode(uint32_t renderPassNode) {
    //防止重复
    if (std::find(renderPassNodes.begin(), renderPassNodes.end(), renderPassNode)
        ==  renderPassNodes.end()) {
        renderPassNodes.push_back(renderPassNode);
    }
    return *this;
}

FG::FrameGraph &FG::FrameGraph::Compile() {
    usingResourceNodes.clear();
    usingPassNodes.clear();
    //清理池
    for (auto &[passIndex, pool]: renderPassCommandPools) {
        if (pool != VK_NULL_HANDLE)
            vkDestroyCommandPool(vulkanRenderAPI.GetVulkanDevice()->logicalDevice,
                                 pool, nullptr);
        pool = VK_NULL_HANDLE;
    }
    timeline.clear();
    //清理别名系统
    resourceManager.ClearAliasGroups();
    CullPassAndResource();
    CreateTimeline();
    CreateAliasGroups();
    InsertBarriers2();
    CreateCommandPools();
    return *this;
}


void FG::FrameGraph::InsertImageBarrier(VkCommandBuffer cmdBuffer, const BarrierInfo &barrier) {
    if (!barrier.isImage) return;
    auto resource = resourceManager.FindResource(barrier.resourceID);
    auto description = resource->GetDescription<TextureDescription>();
    auto texture = vulkanRenderAPI.getByIndex<FrameWork::Texture>(
        resourceManager.GetVulkanIndex(barrier.resourceID));



    VkImageSubresourceRange subresourceRange{};
    subresourceRange.aspectMask = (description->usages & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
                                      ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT
                                      : VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.layerCount = description->arrayLayers;
    subresourceRange.levelCount = description->mipLevels;
    //这里是保证在MSAA时，mipLevel对应是resolve
    subresourceRange.baseMipLevel = 0;
    subresourceRange.baseArrayLayer = 0;

    VkImageMemoryBarrier vkBarrier{};
    vkBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    vkBarrier.image = texture->image.image;
    vkBarrier.oldLayout = barrier.oldLayout;
    vkBarrier.newLayout = barrier.newLayout;
    vkBarrier.srcAccessMask = barrier.srcAccessMask;
    vkBarrier.dstAccessMask = barrier.dstAccessMask;
    vkBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    vkBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    vkBarrier.subresourceRange = subresourceRange;

    vkCmdPipelineBarrier(cmdBuffer,
                         barrier.srcStageMask,
                         barrier.dstStageMask,
                         0,
                         0, nullptr,
                         0, nullptr,
                         1, &vkBarrier);

}

FG::FrameGraph &FG::FrameGraph::Execute(const VkCommandBuffer &commandBuffer) {
    struct RenderPassData {
        VkCommandBuffer secondaryCmd{};
        std::vector<VkRenderingAttachmentInfo> colorAttachments{};
        VkRenderingAttachmentInfo depthAttachment{};
        std::vector<VkFormat> colorFormats{};
        VkFormat depthFormat{VK_FORMAT_UNDEFINED};
        bool hasDepth{};
        uint32_t width{};
        uint32_t height{};
        uint32_t passIndex{};
        uint32_t arrayLayer{1};
        PassType passType{};
    };

    updateBeforeRendering();
    std::vector<std::future<RenderPassData> > futures;
    futures.reserve(usingPassNodes.size());
    resourceManager.ResetVulkanResources();
    resourceManager.CreateVulkanResources(threadPool);

    // 为每个 pass 创建 secondary command buffer
    for (auto &t: timeline) {
        for (auto &passIndex: t) {
            auto renderPass = renderPassManager.FindRenderPass(passIndex);
            auto type = renderPass->GetPassType();
            if (type == PassType::Graphics) {
                futures.push_back(threadPool.Enqueue([this, renderPass, passIndex]() -> RenderPassData {
                    RenderPassData data;
                    data.passIndex = passIndex;
                    data.hasDepth = false;
                    data.width = vulkanRenderAPI.windowWidth;
                    data.height = vulkanRenderAPI.windowHeight;
                    data.passType = PassType::Graphics;
                    data.secondaryCmd = this->commandBuffers[passIndex][vulkanRenderAPI.GetCurrentFrame()];
                    // 收集附件格式信息
                    VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;

                    auto &resourceCreateIndices = renderPass->GetCreateResources();
                    auto &resourceInputIndices = renderPass->GetInputResources();

                    // 处理创建的资源
                    for (auto &resourceIndex: resourceCreateIndices) {
                        auto resource = resourceManager.FindResource(resourceIndex);

                        if (resource && resource->GetType() == ResourceType::Texture) {
                            auto description = resource->GetDescription<TextureDescription>();
                            sampleCount = description->samples;
                            data.width = description->width;
                            data.height = description->height;
                            data.arrayLayer = description->arrayLayers;

                            if ((description->usages & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) ==
                                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
                                data.colorFormats.push_back(description->format);
                                data.colorAttachments.push_back(this->CreateCreateAttachmentInfo(resourceIndex));
                            } else if ((description->usages & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) ==
                                       VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                                data.depthFormat = description->format;
                                data.depthAttachment = this->CreateCreateAttachmentInfo(resourceIndex);
                                data.hasDepth = true;
                            }
                        }
                    }

                    // 处理输入资源
                    for (auto &resourceIndex: resourceInputIndices) {
                        auto resource = resourceManager.FindResource(resourceIndex);
                        if (resource && resource->GetType() == ResourceType::Texture) {
                            auto description = resource->GetDescription<TextureDescription>();
                            sampleCount = description->samples;
                            data.width = description->width;
                            data.height = description->height;
                            data.arrayLayer = description->arrayLayers;
                            if (description->usages & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
                                data.colorFormats.push_back(description->format);
                                data.colorAttachments.push_back(this->CreateInputAttachmentInfo(resourceIndex));
                            } else if ((description->usages & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) ==
                                       VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                                data.depthFormat = description->format;
                                data.depthAttachment = this->CreateInputAttachmentInfo(resourceIndex);
                                data.hasDepth = true;
                            }
                        }
                    }

                    // 设置继承信息
                    VkCommandBufferInheritanceRenderingInfo inheritanceRenderingInfo{};
                    inheritanceRenderingInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDERING_INFO;
                    inheritanceRenderingInfo.colorAttachmentCount = static_cast<uint32_t>(data.colorFormats.size());
                    inheritanceRenderingInfo.pColorAttachmentFormats = data.colorFormats.empty()
                                                                           ? nullptr
                                                                           : data.colorFormats.data();
                    inheritanceRenderingInfo.depthAttachmentFormat = data.depthFormat;
                    inheritanceRenderingInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
                    inheritanceRenderingInfo.rasterizationSamples = sampleCount;
                    inheritanceRenderingInfo.viewMask = 0;

                    VkCommandBufferInheritanceInfo inheritanceInfo{};
                    inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
                    inheritanceInfo.pNext = &inheritanceRenderingInfo;
                    inheritanceInfo.renderPass = VK_NULL_HANDLE;
                    inheritanceInfo.framebuffer = VK_NULL_HANDLE;

                    // 开始记录 Secondary Command Buffer
                    VkCommandBufferBeginInfo cmdBufInfo{};
                    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                    cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
                    cmdBufInfo.pInheritanceInfo = &inheritanceInfo;

                    vkBeginCommandBuffer(data.secondaryCmd, &cmdBufInfo);

                    // 设置 Viewport 和 Scissor
                    VkViewport viewport{};
                    viewport.x = 0.0f;
                    viewport.y = 0.0f;
                    viewport.width = static_cast<float>(data.width);
                    viewport.height = static_cast<float>(data.height);
                    viewport.minDepth = 0.0f;
                    viewport.maxDepth = 1.0f;
                    vkCmdSetViewport(data.secondaryCmd, 0, 1, &viewport);

                    VkRect2D scissor{};
                    scissor.offset = {0, 0};
                    scissor.extent.width = data.width;
                    scissor.extent.height = data.height;
                    vkCmdSetScissor(data.secondaryCmd, 0, 1, &scissor);

                    // 执行渲染 Pass（不包括 barriers）
                    renderPass->GetExec()(data.secondaryCmd);

                    vkEndCommandBuffer(data.secondaryCmd);

                    return data;
                }));
            }else if (type == PassType::Compute) {
                futures.push_back(threadPool.Enqueue([this, renderPass, passIndex]() -> RenderPassData {
                    RenderPassData data;
                    data.passIndex = passIndex;
                    data.hasDepth = false;
                    data.width = vulkanRenderAPI.windowWidth;
                    data.height = vulkanRenderAPI.windowHeight;
                    data.passType = PassType::Compute;
                    data.secondaryCmd = this->commandBuffers[passIndex][vulkanRenderAPI.GetCurrentFrame()];
                    // 收集附件格式信息
                    // 不设置继承信息

                    VkCommandBufferInheritanceInfo inherit{};
                    inherit.sType       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
                    inherit.renderPass  = VK_NULL_HANDLE;    // 计算不在 RenderPass/Rendering 内
                    inherit.subpass     = 0;
                    inherit.framebuffer = VK_NULL_HANDLE;
                    inherit.pNext       = nullptr;
                    // 开始记录 Secondary Command Buffer
                    VkCommandBufferBeginInfo cmdBufInfo{};
                    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                    cmdBufInfo.flags = 0;
                    cmdBufInfo.pInheritanceInfo = &inherit;

                    vkBeginCommandBuffer(data.secondaryCmd, &cmdBufInfo);
                    // 执行渲染 Pass（不包括 barriers）
                    renderPass->GetExec()(data.secondaryCmd);
                    vkEndCommandBuffer(data.secondaryCmd);

                    return data;
                }));
            }else if (type == PassType::Resolve) {
                futures.push_back(threadPool.Enqueue([this, renderPass, passIndex]() -> RenderPassData {
                    RenderPassData data;
                    data.passIndex = passIndex;
                    data.hasDepth = false;
                    data.width = vulkanRenderAPI.windowWidth;
                    data.height = vulkanRenderAPI.windowHeight;
                    data.passType = PassType::Resolve;
                    data.secondaryCmd = this->commandBuffers[passIndex][vulkanRenderAPI.GetCurrentFrame()];
                    // 收集附件格式信息
                    // 不设置继承信息

                    VkCommandBufferInheritanceInfo inherit{};
                    inherit.sType       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
                    inherit.renderPass  = VK_NULL_HANDLE;    // 计算不在 RenderPass/Rendering 内
                    inherit.subpass     = 0;
                    inherit.framebuffer = VK_NULL_HANDLE;
                    inherit.pNext       = nullptr;
                    // 开始记录 Secondary Command Buffer
                    VkCommandBufferBeginInfo cmdBufInfo{};
                    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                    cmdBufInfo.flags = 0;
                    cmdBufInfo.pInheritanceInfo = &inherit;

                    vkBeginCommandBuffer(data.secondaryCmd, &cmdBufInfo);
                    // 执行渲染 Pass（不包括 barriers）
                    renderPass->GetExec()(data.secondaryCmd);
                    vkEndCommandBuffer(data.secondaryCmd);
                    return data;
                }));
            }
        }
    }


    int futureIndex = 0;
    for (auto &t: timeline) {
        for (auto &passIndex: t) {
            auto renderPass = renderPassManager.FindRenderPass(passIndex);
            RenderPassData data = futures[futureIndex++].get();
            VkRenderingInfo renderingInfo = {};
            auto &preBarriers = renderPass->GetPreBarriers();
            for (auto &barrier: preBarriers) {
                if (barrier.isImage)
                    InsertImageBarrier(commandBuffer, barrier);
            }

            if (data.passType == PassType::Compute || data.passType == PassType::Resolve) {
                    vkCmdExecuteCommands(commandBuffer, 1, &data.secondaryCmd);
            } else if (data.passType == PassType::Graphics) {
                if (!data.colorAttachments.empty() || data.hasDepth) {
                    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
                    renderingInfo.pNext = nullptr;
                    renderingInfo.flags = VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT; //告訴他在二級緩衝區錄製
                    renderingInfo.renderArea.offset = {0, 0};
                    renderingInfo.renderArea.extent = {data.width, data.height};
                    renderingInfo.layerCount = data.arrayLayer;
                    renderingInfo.viewMask = 0;
                    renderingInfo.colorAttachmentCount = static_cast<uint32_t>(data.colorAttachments.size());
                    renderingInfo.pColorAttachments =
                            data.colorAttachments.empty() ? nullptr : data.colorAttachments.data();
                    renderingInfo.pDepthAttachment = data.hasDepth ? &data.depthAttachment : nullptr;
                    renderingInfo.pStencilAttachment = nullptr;

                    vkCmdBeginRendering(commandBuffer, &renderingInfo);
                    vkCmdExecuteCommands(commandBuffer, 1, &data.secondaryCmd);
                    vkCmdEndRendering(commandBuffer);
                }
            }


            auto &postBarriers = renderPass->GetPostBarriers();
            for (auto &barrier: postBarriers) {
                if (barrier.isImage)
                    InsertImageBarrier(commandBuffer, barrier);
            }
        }
    }

    //更新释放别名池
    resourceManager.UpdateReusePool();

    return *this;
}

//保留因为验证层不会输出在secondary的报错
// FG::FrameGraph &FG::FrameGraph::Execute(const VkCommandBuffer &commandBuffer) {
//     struct RenderPassData {
//         VkCommandBuffer secondaryCmd{};
//         std::vector<VkRenderingAttachmentInfo> colorAttachments{};
//         VkRenderingAttachmentInfo depthAttachment{};
//         std::vector<VkFormat> colorFormats{};
//         VkFormat depthFormat{VK_FORMAT_UNDEFINED};
//         bool hasDepth{};
//         uint32_t width{};
//         uint32_t height{};
//         uint32_t passIndex{};
//         uint32_t arrayLayer{1};
//         PassType passType{};
//     };
//
//     updateBeforeRendering();
//     resourceManager.ResetVulkanResources();
//     resourceManager.CreateVulkanResources(threadPool);
//
//     auto buildGraphicsPassData = [&](uint32_t passIndex) -> RenderPassData {
//         RenderPassData data;
//         auto renderPass = renderPassManager.FindRenderPass(passIndex);
//         data.passIndex = passIndex;
//         data.hasDepth  = false;
//         data.width     = vulkanRenderAPI.windowWidth;
//         data.height    = vulkanRenderAPI.windowHeight;
//         data.passType  = PassType::Graphics;
//         data.secondaryCmd = this->commandBuffers[passIndex][vulkanRenderAPI.GetCurrentFrame()];
//
//         VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;
//
//         auto &resourceCreateIndices = renderPass->GetCreateResources();
//         auto &resourceInputIndices  = renderPass->GetInputResources();
//
//         // 处理创建的资源
//         for (auto &resourceIndex : resourceCreateIndices) {
//             auto resource = resourceManager.FindResource(resourceIndex);
//             if (resource && resource->GetType() == ResourceType::Texture) {
//                 auto description = resource->GetDescription<TextureDescription>();
//                 sampleCount      = description->samples;
//                 data.width       = description->width;
//                 data.height      = description->height;
//                 data.arrayLayer  = description->arrayLayers;
//
//                 if ((description->usages & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) ==
//                     VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
//                     data.colorFormats.push_back(description->format);
//                     data.colorAttachments.push_back(this->CreateCreateAttachmentInfo(resourceIndex));
//                 } else if ((description->usages & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) ==
//                            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
//                     data.depthFormat   = description->format;
//                     data.depthAttachment = this->CreateCreateAttachmentInfo(resourceIndex);
//                     data.hasDepth = true;
//                 }
//             }
//         }
//
//         // 处理输入资源
//         for (auto &resourceIndex : resourceInputIndices) {
//             auto resource = resourceManager.FindResource(resourceIndex);
//             if (resource && resource->GetType() == ResourceType::Texture) {
//                 auto description = resource->GetDescription<TextureDescription>();
//                 sampleCount      = description->samples;
//                 data.width       = description->width;
//                 data.height      = description->height;
//                 data.arrayLayer  = description->arrayLayers;
//
//                 if (description->usages & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
//                     data.colorFormats.push_back(description->format);
//                     data.colorAttachments.push_back(this->CreateInputAttachmentInfo(resourceIndex));
//                 } else if ((description->usages & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) ==
//                            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
//                     data.depthFormat   = description->format;
//                     data.depthAttachment = this->CreateInputAttachmentInfo(resourceIndex);
//                     data.hasDepth = true;
//                 }
//             }
//         }
//
//         // 设置继承信息
//         VkCommandBufferInheritanceRenderingInfo inheritanceRenderingInfo{};
//         inheritanceRenderingInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDERING_INFO;
//         inheritanceRenderingInfo.colorAttachmentCount     = static_cast<uint32_t>(data.colorFormats.size());
//         inheritanceRenderingInfo.pColorAttachmentFormats  = data.colorFormats.empty() ? nullptr : data.colorFormats.data();
//         inheritanceRenderingInfo.depthAttachmentFormat    = data.depthFormat;
//         inheritanceRenderingInfo.stencilAttachmentFormat  = VK_FORMAT_UNDEFINED;
//         inheritanceRenderingInfo.rasterizationSamples     = sampleCount;
//         inheritanceRenderingInfo.viewMask                 = 0;
//
//         VkCommandBufferInheritanceInfo inheritanceInfo{};
//         inheritanceInfo.sType                = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
//         inheritanceInfo.pNext                = &inheritanceRenderingInfo;
//         inheritanceInfo.renderPass           = VK_NULL_HANDLE;
//         inheritanceInfo.framebuffer          = VK_NULL_HANDLE;
//
//         // 开始记录 Secondary Command Buffer
//         VkCommandBufferBeginInfo cmdBufInfo{};
//         cmdBufInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
//         cmdBufInfo.flags            = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
//         cmdBufInfo.pInheritanceInfo = &inheritanceInfo;
//
//         vkBeginCommandBuffer(data.secondaryCmd, &cmdBufInfo);
//
//         // 设置 Viewport 和 Scissor
//         VkViewport viewport{};
//         viewport.x        = 0.0f;
//         viewport.y        = 0.0f;
//         viewport.width    = static_cast<float>(data.width);
//         viewport.height   = static_cast<float>(data.height);
//         viewport.minDepth = 0.0f;
//         viewport.maxDepth = 1.0f;
//         vkCmdSetViewport(data.secondaryCmd, 0, 1, &viewport);
//
//         VkRect2D scissor{};
//         scissor.offset = {0, 0};
//         scissor.extent = {data.width, data.height};
//         vkCmdSetScissor(data.secondaryCmd, 0, 1, &scissor);
//
//         // 执行渲染 Pass（不包括 barriers）
//         renderPass->GetExec()(data.secondaryCmd);
//
//         vkEndCommandBuffer(data.secondaryCmd);
//
//         return data;
//     };
//
//     auto buildComputePassData = [&](uint32_t passIndex) -> RenderPassData {
//         RenderPassData data;
//         auto renderPass = renderPassManager.FindRenderPass(passIndex);
//         data.passIndex  = passIndex;
//         data.hasDepth   = false;
//         data.width      = vulkanRenderAPI.windowWidth;
//         data.height     = vulkanRenderAPI.windowHeight;
//         data.passType   = PassType::Compute;
//         data.secondaryCmd = this->commandBuffers[passIndex][vulkanRenderAPI.GetCurrentFrame()];
//
//         VkCommandBufferInheritanceInfo inherit{};
//         inherit.sType       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
//         inherit.renderPass  = VK_NULL_HANDLE;
//         inherit.subpass     = 0;
//         inherit.framebuffer = VK_NULL_HANDLE;
//         inherit.pNext       = nullptr;
//
//         VkCommandBufferBeginInfo cmdBufInfo{};
//         cmdBufInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
//         cmdBufInfo.flags            = 0;
//         cmdBufInfo.pInheritanceInfo = &inherit;
//
//         vkBeginCommandBuffer(data.secondaryCmd, &cmdBufInfo);
//         renderPass->GetExec()(data.secondaryCmd);
//         vkEndCommandBuffer(data.secondaryCmd);
//
//         return data;
//     };
//
//     // 单线程执行：按 timeline 顺序逐个构建并执行
//     for (auto &t : timeline) {
//         for (auto &passIndex : t) {
//             auto renderPass = renderPassManager.FindRenderPass(passIndex);
//
//             // Pre-barriers
//             auto &preBarriers = renderPass->GetPreBarriers();
//             for (auto &barrier : preBarriers) {
//                 if (barrier.isImage) {
//                     InsertImageBarrier(commandBuffer, barrier);
//                 }
//             }
//
//             // 构建 secondary 并执行
//             if (renderPass->GetPassType() == PassType::Compute) {
//                 RenderPassData data = buildComputePassData(passIndex);
//                 vkCmdExecuteCommands(commandBuffer, 1, &data.secondaryCmd);
//             } else {
//                 RenderPassData data = buildGraphicsPassData(passIndex);
//
//                 if (!data.colorAttachments.empty() || data.hasDepth) {
//                     VkRenderingInfo renderingInfo{};
//                     renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
//                     renderingInfo.pNext = nullptr;
//                     renderingInfo.flags = VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT;
//                     renderingInfo.renderArea.offset = {0, 0};
//                     renderingInfo.renderArea.extent = {data.width, data.height};
//                     renderingInfo.layerCount = data.arrayLayer;
//                     renderingInfo.viewMask   = 0;
//                     renderingInfo.colorAttachmentCount = static_cast<uint32_t>(data.colorAttachments.size());
//                     renderingInfo.pColorAttachments    = data.colorAttachments.empty() ? nullptr : data.colorAttachments.data();
//                     renderingInfo.pDepthAttachment     = data.hasDepth ? &data.depthAttachment : nullptr;
//                     renderingInfo.pStencilAttachment   = nullptr;
//
//                     vkCmdBeginRendering(commandBuffer, &renderingInfo);
//                     vkCmdExecuteCommands(commandBuffer, 1, &data.secondaryCmd);
//                     vkCmdEndRendering(commandBuffer);
//                 }
//             }
//
//             // Post-barriers
//             auto &postBarriers = renderPass->GetPostBarriers();
//             for (auto &barrier : postBarriers) {
//                 if (barrier.isImage) {
//                     InsertImageBarrier(commandBuffer, barrier);
//                 }
//             }
//         }
//     }
//
//     // 更新释放别名池
//     resourceManager.UpdateReusePool();
//
//     return *this;
// }


FG::FrameGraph &FG::FrameGraph::SetUpdateBeforeRendering(const std::function<void()> &callback) {
    updateBeforeRendering = callback;
    return *this;
}

void FG::FrameGraph::CullPassAndResource() {
    std::unordered_set<uint32_t> usePassNode;
    std::vector<uint32_t> startPassNodes;

    //先根据renderPass的信息构建Resource的拓扑关系
    for (auto &renderPassID: renderPassNodes) {
        auto renderPass = renderPassManager.FindRenderPass(renderPassID);
        for (auto &id: renderPass->GetCreateResources()) {
            auto resource = resourceManager.FindResource(id);
            resource->SetOutputRenderPass(renderPassID);
        }
        for (auto &id: renderPass->GetOutputResources()) {
            auto resource = resourceManager.FindResource(id);
            resource->SetOutputRenderPass(renderPassID);
        }
        for (auto &id: renderPass->GetInputResources()) {
            auto resource = resourceManager.FindResource(id);
            resource->SetInputRenderPass(renderPassID);
        }
        for (auto &id: renderPass->GetReadResources()) {
            auto resource = resourceManager.FindResource(id);
            resource->SetInputRenderPass(renderPassID);
        }
    }

    //找到write 或者 proxy 的RenderPass作为反向的起点
    for (auto &renderPassIndex: renderPassNodes) {
        auto renderPassNode = renderPassManager.FindRenderPass(renderPassIndex);
        auto createResources = renderPassNode->GetCreateResources();
        auto outputAttachments = renderPassNode->GetOutputResources();
        if (!createResources.empty() || !outputAttachments.empty()) {
            bool found = false;
            for (auto createResourceIndex: createResources) {
                auto resource = resourceManager.FindResource(createResourceIndex);
                if (resource) {
                    if (resource->isExternal) {
                        usePassNode.insert(renderPassIndex);
                        startPassNodes.push_back(renderPassIndex);
                        found = true;
                        break;
                    }
                }
            }
            if (!found)
                for (auto writeResourceIndex: outputAttachments) {
                    auto resource = resourceManager.FindResource(writeResourceIndex);
                    if (resource) {
                        if (resource->isExternal) {
                            usePassNode.insert(renderPassIndex);
                            startPassNodes.push_back(renderPassIndex);
                            break;
                        }
                    }
                }
        }
    }

    std::function<void(uint32_t)> search = [&](uint32_t start) {
        auto startPass = renderPassManager.FindRenderPass(start);
        if (startPass != nullptr) {
            auto readResources = startPass->GetReadResources();
            for (auto &readResourceIndex: readResources) {
                auto readResource = resourceManager.FindResource(readResourceIndex);
                for (auto &renderPassIndex: readResource->GetOutputRenderPass()) {
                    if (!usePassNode.contains(renderPassIndex)) {
                        usePassNode.insert(renderPassIndex);
                        search(renderPassIndex);
                    }
                }
            }
            auto inputResources = startPass->GetInputResources();
            for (auto &inputResourceIndex: inputResources) {
                auto readResource = resourceManager.FindResource(inputResourceIndex);
                for (auto &renderPassIndex: readResource->GetOutputRenderPass()) {
                    if (!usePassNode.contains(renderPassIndex)) {
                        usePassNode.insert(renderPassIndex);
                        search(renderPassIndex);
                    }
                }
            }
        }
    };

    //开始反向搜索
    for (auto startPassNode: startPassNodes) {
        search(startPassNode);
    }

    //获得工作resource
    for (auto resourceIndex: resourceNodes) {
        auto resource = resourceManager.FindResource(resourceIndex);
        bool isDepth = false;
        if (resource->GetType() == ResourceType::Texture) {
            auto desc = resource->GetDescription<TextureDescription>();
            if (desc->usages & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                isDepth = true;
            }
        }
        if (resource->isExternal) {
            usingResourceNodes.push_back(resourceIndex);
        } else {
            for (auto &renderPassIndex: resource->GetInputRenderPass()) {
                if (usePassNode.contains(renderPassIndex)) {
                    usingResourceNodes.push_back(resourceIndex);
                }
            }
            if (isDepth)
                for (auto &renderPassIndex: resource->GetOutputRenderPass()) {
                    if (usePassNode.contains(renderPassIndex) && resource->GetInputRenderPass().empty()) {
                        usingResourceNodes.push_back(resourceIndex);
                    }
                }
        }
    }

    //获得工作pass
    for (auto &renderPassNode: renderPassNodes) {
        if (usePassNode.contains(renderPassNode)) {
            usingPassNodes.push_back(renderPassNode);
        }
    }
}

void FG::FrameGraph::CreateTimeline() {
    //创建依赖关系
    std::vector<uint32_t> q;
    std::unordered_set<uint32_t> hasAddedPass;
    for (auto &renderPassIndex: usingPassNodes) {
        auto renderPass = renderPassManager.FindRenderPass(renderPassIndex);
        if (renderPass != nullptr) {
            //清空依赖保证安全
            renderPass->GetRenderPassDependencies().clear();
            for (auto &inputIndex: renderPass->GetInputResources()) {
                auto input = resourceManager.FindResource(inputIndex);
                if (input != nullptr) {
                    for (auto &inputRenderPassIndex: input->GetOutputRenderPass()) {
                        renderPass->AddRenderPassDependency(inputRenderPassIndex);
                    }
                }
            }
            for (auto &readIndex: renderPass->GetReadResources()) {
                auto readResource = resourceManager.FindResource(readIndex);
                if (readResource != nullptr) {
                    for (auto &inputRenderPassIndex: readResource->GetOutputRenderPass()) {
                        renderPass->AddRenderPassDependency(inputRenderPassIndex);
                    }
                }
            }
            if (renderPass->GetRenderPassDependencies().empty()) {
                q.push_back(renderPassIndex);
                hasAddedPass.insert(renderPassIndex);
            }
        }
    }
    // // DeBug
    //  for (auto &renderPassNode: renderPassNodes) {
    //      auto renderPass = renderPassManager.FindRenderPass(renderPassNode);
    //
    //      std::vector<std::string> dep;
    //      for (auto& dependency : renderPass->GetRenderPassDependencies()) {
    //          dep.push_back(renderPassManager.FindRenderPass(dependency)->GetName());
    //      }
    //      LOG_DEBUG("RenderPass : {} Dependencies are {}", renderPass->GetName(), dep);
    //  }
    while (!q.empty()) {
        timeline.push_back(q);
        std::vector<uint32_t> tempQ;
        for (auto &renderPassIndex: usingPassNodes) {
            if (hasAddedPass.contains(renderPassIndex)) {
                continue;
            }
            auto renderPass = renderPassManager.FindRenderPass(renderPassIndex);
            if (renderPass != nullptr) {
                auto dependencies = renderPass->GetRenderPassDependencies();
                for (auto &r: hasAddedPass) {
                    if (dependencies.contains(r)) {
                        dependencies.erase(r);
                    }
                }
                if (dependencies.empty()) {
                    tempQ.push_back(renderPassIndex);
                    hasAddedPass.insert(renderPassIndex);
                }
            }
        }
        q = std::move(tempQ);
    }

    if (hasAddedPass.size() != usingPassNodes.size()) {
        LOG_ERROR("There are some RenderPass have cyclic dependencies !");
        //终止
        return;
    }

    //调整资源在时间线中的使用情况
    for (auto &resourceIndex: usingResourceNodes) {
        auto resource = resourceManager.FindResource(resourceIndex);
        resource->ResetUseTime();
        bool first = false;
        for (int i = 0; i < timeline.size(); ++i) {
            for (auto &renderPassIndex: resource->GetOutputRenderPass()) {
                if (std::ranges::find(timeline[i], renderPassIndex) != std::ranges::end(timeline[i])) {
                    resource->SetFirstUseTime(i);
                    first = true;
                    break;
                }
            }
            if (first) {
                break;
            }
        }
        if (!first && resource->isExternal == false) {
            LOG_ERROR("the resource : {} don't have output RenderPass", resource->GetName());
            return;
        }
        bool last = false;
        for (int i = timeline.size() - 1; i >= 0; i--) {
            for (auto &renderPassIndex: resource->GetInputRenderPass()) {
                if (std::ranges::find(timeline[i], renderPassIndex) != std::ranges::end(timeline[i])) {
                    resource->SetLastUseTime(i);
                    last = true;
                    break;
                }
            }
            if (last) {
                break;
            }
        }
        if (!last && resource->isExternal == false) {
            if (resource->GetType() == ResourceType::Texture) {
                auto desc = resource->GetDescription<TextureDescription>();
                if (!(desc->usages & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)) {
                    LOG_ERROR("the resource : {} don't have input RenderPass", resource->GetName());
                    return;
                }
            }
        }
    }
}

void FG::FrameGraph::CreateAliasGroups() {
    std::vector<uint32_t> tempResourceNode;
    for (auto &node: usingResourceNodes) {
        //外部资源不处理
        if (!resourceManager.FindResource(node)->isExternal) {
            tempResourceNode.push_back(node);
        }
    }
    //首先排序根据第一次使用的时间
    std::ranges::sort(tempResourceNode,
                      [&](uint32_t a, uint32_t b) {
                          return resourceManager.FindResource(a)->GetFirstUseTime() <
                                 resourceManager.FindResource(b)->GetFirstUseTime();
                      }
    );

    //贪心创建资源
    for (auto &resIndex: tempResourceNode) {
        auto resource = resourceManager.FindResource(resIndex);
        if (resource != nullptr) {
            //首先先将input和output一一对应
            bool foundAlias = false;
            for (auto &renderPassIndex: resource->GetOutputRenderPass()) {
                auto renderPass = renderPassManager.FindRenderPass(renderPassIndex);
                if (renderPass != nullptr) {
                    if (renderPass->GetInputResources().size() != renderPass->GetOutputResources().size()) {
                        LOG_ERROR("The RenderPass name : {} input size not equal output size !", renderPass->GetName());
                        return;
                    }
                    int Found = -1;
                    for (int i = 0; i < renderPass->GetOutputResources().size(); i++) {
                        if (renderPass->GetOutputResources()[i] == resIndex) {
                            Found = i;
                            foundAlias = true;
                            break;
                        }
                    }
                    if (Found != -1) {
                        auto inputResourceIndex = renderPass->GetInputResources()[Found];
                        auto inputResource = resourceManager.FindResource(inputResourceIndex);

                        if (inputResource->GetType() != resource->GetType()) {
                            LOG_ERROR("The RenderPass name : {} input type not equal output type !",
                                      renderPass->GetName());
                        }
                        if (!resourceManager.resourceDescriptionToAliasGroup.contains(inputResourceIndex)) {
                            LOG_ERROR("Find the input resource {} didn't create alias than out resource {}",
                                      inputResource->GetName(), resource->GetName());
                        }
                        auto &aliasGroup = resourceManager.aliasGroups[resourceManager.resourceDescriptionToAliasGroup[
                            inputResourceIndex]];
                        aliasGroup->sharedResourceIndices.push_back(resIndex);
                        resourceManager.resourceDescriptionToAliasGroup[resIndex] = resourceManager.
                                resourceDescriptionToAliasGroup[inputResourceIndex];
                    }
                }
            }
            if (!foundAlias) {
                auto &aliasGroups = resourceManager.GetAliasGroups();
                for (int i = 0; i < aliasGroups.size(); i++) {
                    if (resourceManager.CanAlias(resIndex, i)) {
                        aliasGroups[i]->sharedResourceIndices.push_back(resIndex);
                        resourceManager.resourceDescriptionToAliasGroup[resIndex] = i;
                        foundAlias = true;
                        break;
                    }
                }
            }
            if (!foundAlias) {
                auto newGroup = std::make_unique<AliasGroup>();
                newGroup->sharedResourceIndices.push_back(resIndex);
                if (resource->GetType() == ResourceType::Texture) {
                    newGroup->description = resource->GetDescription<TextureDescription>();
                } else {
                    newGroup->description = resource->GetDescription<BufferDescription>();
                }
                newGroup->vulkanIndex = -1; //在运行时创建资源
                resourceManager.GetAliasGroups().push_back(std::move(newGroup));
                resourceManager.resourceDescriptionToAliasGroup[resIndex] =
                        resourceManager.GetAliasGroups().size() - 1;
            }
        }
    }
}

void FG::FrameGraph::CreateCommandPools() {
    //这里先不考虑compute，计算着色器需要不同queue
    renderPassCommandPools.clear();
    for (auto &renderPassIndex: usingPassNodes) {
        VkCommandPoolCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        info.queueFamilyIndex = vulkanRenderAPI.GetVulkanDevice()->queueFamilyIndices.graphics;
        info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        VK_CHECK_RESULT(
            vkCreateCommandPool(vulkanRenderAPI.GetVulkanDevice()->logicalDevice, &info, nullptr,
                &renderPassCommandPools[renderPassIndex])
        );
        commandBuffers[renderPassIndex].resize(MAX_FRAME);
        for (int i = 0; i < MAX_FRAME; i++) {
            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = renderPassCommandPools[renderPassIndex];
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
            allocInfo.commandBufferCount = 1;

            vkAllocateCommandBuffers(vulkanRenderAPI.GetVulkanDevice()->logicalDevice,
                                     &allocInfo, &commandBuffers[renderPassIndex][i]);
        }
    }
}


VkRenderingAttachmentInfo FG::FrameGraph::CreateInputAttachmentInfo(uint32_t resourceIndex) {
    auto resourceDesc = resourceManager.FindResource(resourceIndex);
    if (resourceDesc->GetType() == ResourceType::Buffer) {
        LOG_ERROR("the resource name: {} is not attachment !", resourceDesc->GetName());
    }

    VkRenderingAttachmentInfo attachmentInfo = {};
    attachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    attachmentInfo.pNext = nullptr; // 重要：明确设置为 nullptr

    auto texture = vulkanRenderAPI.getByIndex<FrameWork::Texture>(resourceManager.GetVulkanIndex(resourceIndex));

    if (!texture || texture->imageView == VK_NULL_HANDLE) {
        LOG_ERROR("Invalid texture or imageView for resource index: {}", resourceIndex);
        return attachmentInfo;
    }

    attachmentInfo.imageView = texture->imageView;
    attachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;


    attachmentInfo.resolveMode = VK_RESOLVE_MODE_NONE;
    attachmentInfo.resolveImageView = VK_NULL_HANDLE;
    attachmentInfo.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if ((texture->image.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) == VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
        attachmentInfo.clearValue.color = vulkanRenderAPI.defaultClearColor;
        attachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    } else if ((texture->image.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) ==
               VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        attachmentInfo.clearValue.depthStencil = {1.0f, 0};
        attachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    }

    return attachmentInfo;
}

FG::ResourceManager & FG::FrameGraph::GetResourceManager() {
    return resourceManager;
}

FG::RenderPassManager & FG::FrameGraph::GetRenderPassManager() {
    return renderPassManager;
}

VkRenderingAttachmentInfo FG::FrameGraph::CreateCreateAttachmentInfo(uint32_t resourceIndex) {
    VkRenderingAttachmentInfo attachmentInfo = {};
    attachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    attachmentInfo.pNext = nullptr; // 重要：明确设置为 nullptr

    auto texture = vulkanRenderAPI.getByIndex<FrameWork::Texture>(resourceManager.GetVulkanIndex(resourceIndex));

    // 添加安全检查
    if (!texture || texture->imageView == VK_NULL_HANDLE) {
        LOG_ERROR("Invalid texture or imageView for resource index: {}", resourceIndex);
        return attachmentInfo; // 返回无效的 attachment
    }

    attachmentInfo.imageView = texture->imageView;
    attachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    // 重要：初始化 resolve 相关字段
    attachmentInfo.resolveMode = VK_RESOLVE_MODE_NONE;
    attachmentInfo.resolveImageView = VK_NULL_HANDLE;
    attachmentInfo.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if ((texture->image.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) == VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
        attachmentInfo.clearValue.color = vulkanRenderAPI.defaultClearColor;
        attachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    } else if ((texture->image.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) ==
               VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        attachmentInfo.clearValue.depthStencil = {1.0f, 0};
        attachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    }

    return attachmentInfo;
}

void FG::FrameGraph::InsertBarriers2() {
    //先只处理Image
    struct ResourceState {
        bool readable = false;
        bool writable = false;
    };
    std::unordered_map<uint32_t, ResourceState> resourceStates;

    struct VulkanState {
        VkAccessFlags accessMasks;
        VkPipelineStageFlags pipelineStages;
        VkImageLayout layout;
        bool operator==(const VulkanState& other) const {
            return pipelineStages == other.pipelineStages && layout == other.layout && accessMasks == other.accessMasks;
        }
    };
    std::unordered_map<uint32_t, VulkanState> preState;//记录的src

    auto SwitchResourceState = [](const ResourceState& r) {
        if (r.readable == false && r.writable == true) { //Create
            return 0;
        }
        if (r.readable == true && r.writable == true) { //InputOutput
            return 1;
        }
        else if (r.readable == true && r.writable == false) { //Read
            return 2;
        }
        LOG_ERROR("不支持不可读不可写状态");
        return -1;
    };

    std::vector<std::vector<std::vector<VulkanState>>> resourceMap;//管线希望转换成的Layout和阶段，也就是dst
    resourceMap.resize(4); //pipeline Type
    for (auto& t0 : resourceMap) {
        t0.resize(3); //w wr r
        for (auto& t1 : t0) {
            t1.resize(2); //aspect
        }
    }
    auto create = 0;
    auto inputOutput = 1;
    auto read = 2;
    auto color = 0;
    auto depth = 1;

    resourceMap[static_cast<int>(PassType::Graphics)][create][color] = {
        .accessMasks = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .pipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    resourceMap[static_cast<int>(PassType::Graphics)][create][depth] = {
        .accessMasks = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .pipelineStages = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    resourceMap[static_cast<int>(PassType::Graphics)][inputOutput][color] = {
        .accessMasks = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
        .pipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    resourceMap[static_cast<int>(PassType::Graphics)][inputOutput][depth] = {
        .accessMasks = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .pipelineStages = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    resourceMap[static_cast<int>(PassType::Graphics)][read][color] = {
        .accessMasks = VK_ACCESS_SHADER_READ_BIT,
        .pipelineStages = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    resourceMap[static_cast<int>(PassType::Graphics)][read][depth] = {
        .accessMasks = VK_ACCESS_SHADER_READ_BIT,
        .pipelineStages = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };


    resourceMap[static_cast<int>(PassType::Compute)][create][color] = { //不太存在Compute只写无读
        .accessMasks = VK_ACCESS_SHADER_WRITE_BIT,
        .pipelineStages = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .layout = VK_IMAGE_LAYOUT_GENERAL,
    };

    resourceMap[static_cast<int>(PassType::Compute)][create][depth] = { //这边是错误的Depth不能这样操作
        .accessMasks = VK_ACCESS_SHADER_WRITE_BIT,
        .pipelineStages = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .layout = VK_IMAGE_LAYOUT_GENERAL,
    };

    resourceMap[static_cast<int>(PassType::Compute)][inputOutput][color] = {
        .accessMasks = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT,
        .pipelineStages = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .layout = VK_IMAGE_LAYOUT_GENERAL,
    };

    resourceMap[static_cast<int>(PassType::Compute)][inputOutput][depth] = {
        .accessMasks = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT,
        .pipelineStages = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .layout = VK_IMAGE_LAYOUT_GENERAL,
    };

    resourceMap[static_cast<int>(PassType::Compute)][read][color] = {
        .accessMasks = VK_ACCESS_SHADER_READ_BIT,
        .pipelineStages = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .layout = VK_IMAGE_LAYOUT_GENERAL,
    };

    resourceMap[static_cast<int>(PassType::Compute)][read][depth] = {
        .accessMasks = VK_ACCESS_SHADER_READ_BIT,
        .pipelineStages = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .layout = VK_IMAGE_LAYOUT_GENERAL,
    };

    //Resolve节点，这个节点比较特殊，就是读
    resourceMap[static_cast<int>(PassType::Resolve)][read][color] = {
        .accessMasks = VK_ACCESS_TRANSFER_READ_BIT,
        .pipelineStages = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
    };

    //Resolve节点写入节点的内容
    resourceMap[static_cast<int>(PassType::Resolve)][create][color] = {
        .accessMasks = VK_ACCESS_TRANSFER_WRITE_BIT,
        .pipelineStages = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    };


    for (auto& passSet : timeline) {
        for (auto& passIndex : passSet) {
            auto pass = renderPassManager.FindRenderPass(passIndex);
            auto passType = pass->GetPassType();
            for (auto& resourceIndex : pass->GetCreateResources()) {
                auto resource = resourceManager.FindResource(resourceIndex);
                auto desc = resource->GetDescription<TextureDescription>();
                auto aspect = desc->usages & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ? 0 : 1;
                if (resource->GetType() == ResourceType::Texture) {
                    resourceStates[resourceIndex].readable = false;
                    resourceStates[resourceIndex].writable = true;
                    auto dst = resourceMap[static_cast<int>(passType)][SwitchResourceState(resourceStates[resourceIndex])][aspect];
                    preState[resourceIndex] = dst;
                    BarrierInfo barrierInfo = {
                        .resourceID = resourceIndex,
                        .srcAccessMask = 0,
                        .dstAccessMask = dst.accessMasks,
                        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                        .newLayout = dst.layout,
                        .srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                        .dstStageMask = dst.pipelineStages,
                        .isImage = true
                    };
                    pass->AddPreBarrier(barrierInfo);
                    //特殊判断一下present
                    if (resource->isPresent) {
                        BarrierInfo barrierInfo2 = {
                            .resourceID = resourceIndex,
                            .srcAccessMask = dst.accessMasks,
                            .dstAccessMask = 0,
                            .oldLayout = dst.layout,
                            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                            .srcStageMask = dst.pipelineStages,
                            .dstStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                            .isImage = true
                        };
                        pass->AddPostBarrier(barrierInfo2);
                    }
                }
            }

            for (auto& resourceIndex : pass->GetInputResources()) {
                auto resource = resourceManager.FindResource(resourceIndex);
                auto desc = resource->GetDescription<TextureDescription>();
                auto aspect = desc->usages & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ? 0 : 1;
                if (resource->GetType() == ResourceType::Texture) {
                    auto prePassType = renderPassManager.FindRenderPass(*resource->GetOutputRenderPass().begin())->GetPassType();
                    auto src = preState.contains(resourceIndex) ? preState[resourceIndex] :
                    resourceMap[static_cast<int>(prePassType)][SwitchResourceState(resourceStates[resourceIndex])][aspect];
                    resourceStates[resourceIndex].readable = true;
                    resourceStates[resourceIndex].writable = true;
                    auto dst = resourceMap[static_cast<int>(passType)][SwitchResourceState(resourceStates[resourceIndex])][aspect];
                    preState[resourceIndex] = dst;
                    BarrierInfo barrierInfo = {
                        .resourceID = resourceIndex,
                        .srcAccessMask = src.accessMasks,
                        .dstAccessMask = dst.accessMasks,
                        .oldLayout = src.layout,
                        .newLayout = dst.layout,
                        .srcStageMask = src.pipelineStages,
                        .dstStageMask = dst.pipelineStages,
                        .isImage = true
                    };
                    pass->AddPreBarrier(barrierInfo); //特别的Input 读和Output本质是一个物理资源，虽然两个节点是为了防止成环
                    auto index = (&resourceIndex - pass->GetInputResources().data());
                    auto outputResourceIndex = pass->GetOutputResources()[index];
                    resourceStates[outputResourceIndex].writable = true;
                    resourceStates[outputResourceIndex].readable = true;
                }
            }

            for (auto& resourceIndex : pass->GetReadResources()) {
                auto resource = resourceManager.FindResource(resourceIndex);
                auto desc = resource->GetDescription<TextureDescription>();
                auto aspect = desc->usages & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ? 0 : 1;
                if (resource->GetType() == ResourceType::Texture) {
                    auto prePassType = renderPassManager.FindRenderPass(*resource->GetOutputRenderPass().begin())->GetPassType();
                    auto src = preState.contains(resourceIndex) ? preState[resourceIndex] :
                        resourceMap[static_cast<int>(prePassType)][SwitchResourceState(resourceStates[resourceIndex])][aspect];
                    resourceStates[resourceIndex].readable = true;
                    resourceStates[resourceIndex].writable = false;
                    auto dst = resourceMap[static_cast<int>(passType)][SwitchResourceState(resourceStates[resourceIndex])][aspect];
                    preState[resourceIndex] = dst;
                    if (dst == src ) continue; //如果完全相同，读后读不需要插入barrier
                    BarrierInfo barrierInfo = {
                        .resourceID = resourceIndex,
                        .srcAccessMask = src.accessMasks,
                        .dstAccessMask = dst.accessMasks,
                        .oldLayout = src.layout,
                        .newLayout = dst.layout,
                        .srcStageMask = src.pipelineStages,
                        .dstStageMask = dst.pipelineStages,
                        .isImage = true
                    };
                    pass->AddPreBarrier(barrierInfo);
                }
            }
        }
    }
}
