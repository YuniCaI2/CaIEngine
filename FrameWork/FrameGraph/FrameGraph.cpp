//
// Created by cai on 2025/9/13.
//

#include "FrameGraph.h"

#include <unordered_set>

#include "ResourceManager.h"

FG::FrameGraph::FrameGraph(ResourceManager &resourceManager, RenderPassManager &renderPassManager):
resourceManager(resourceManager), renderPassManager(renderPassManager), threadPool(8)
{}

FG::FrameGraph::~FrameGraph() {
    // 清理command pools
    for (auto& [passIndex, pools] : renderPassCommandPools) {
        for (auto& pool : pools) {
            if(pool != VK_NULL_HANDLE)
            vkDestroyCommandPool(vulkanRenderAPI.GetVulkanDevice()->logicalDevice,
                                pool, nullptr);
            pool = VK_NULL_HANDLE;
        }
    }
    renderPassCommandPools.clear();
}

FG::FrameGraph& FG::FrameGraph::AddResourceNode(uint32_t resourceNode) {
    resourceNodes.push_back(resourceNode);
    return *this;
}

FG::FrameGraph& FG::FrameGraph::AddRenderPassNode(uint32_t renderPassNode) {
    renderPassNodes.push_back(renderPassNode);
    return *this;
}

FG::FrameGraph& FG::FrameGraph::Compile() {
    // timeline.clear();
    // usingPassNodes.clear();
    // usingResourceNodes.clear();
    //
    // // 清理command pools
    // for (auto& [passIndex, pool] : renderPassCommandPools) {
    //     if(pool != VK_NULL_HANDLE)
    //     vkDestroyCommandPool(vulkanRenderAPI.GetVulkanDevice()->logicalDevice,
    //                         pool, nullptr);
    //     pool = VK_NULL_HANDLE;
    // }
    // renderPassCommandPools.clear();
    //
    // // 清理资源的alias信息
    // resourceManager.ClearAliasGroups();
    //
    // // 清理每个pass的barriers
    // for (auto& passIndex : renderPassNodes) {
    //     auto pass = renderPassManager.FindRenderPass(passIndex);
    //     if (pass) {
    //         pass->ClearBarriers();
    //     }
    // }

    CullPassAndResource();
    CreateTimeline();
    CreateAliasGroups();
    InsertBarriers(); 
    CreateCommandPools();
    return *this;
}

void FG::FrameGraph::ResetCommandPool(){
    for(auto& [passIndex, pools] : renderPassCommandPools){
        if(pools[vulkanRenderAPI.currentFrame] != VK_NULL_HANDLE){
            vkResetCommandPool(vulkanRenderAPI.GetVulkanDevice()->logicalDevice, pools[vulkanRenderAPI.currentFrame], 0);
        }
    }
}

void FG::FrameGraph::InsertImageBarrier(VkCommandBuffer cmdBuffer, const BarrierInfo &barrier){
    if (!barrier.isImage) return;



    auto resource = resourceManager.FindResource(barrier.resourceID);
    auto description = resource->GetDescription<TextureDescription>();

    VkImageSubresourceRange subresourceRange{};
    subresourceRange.aspectMask = (description->usages & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
        ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.layerCount = description->arrayLayers;
    subresourceRange.levelCount = description->mipLevels;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.baseArrayLayer = 0;

    VkImageMemoryBarrier vkBarrier{};
    vkBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    vkBarrier.image = vulkanRenderAPI.getByIndex<FrameWork::Texture>(
        resourceManager.GetVulkanResource(barrier.resourceID))->image.image;
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

FG::FrameGraph& FG::FrameGraph::Execute(const VkCommandBuffer& commandBuffer) {
    struct RenderPassData {
        VkCommandBuffer secondaryCmd;
        std::vector<VkRenderingAttachmentInfo> colorAttachments;
        VkRenderingAttachmentInfo depthAttachment;
        std::vector<VkFormat> colorFormats;
        VkFormat depthFormat{VK_FORMAT_UNDEFINED};
        bool hasDepth;
        uint32_t width;
        uint32_t height;
        uint32_t passIndex;
        uint32_t arrayLayer{1};
    };

    updateBeforeRendering();
    std::vector<std::future<RenderPassData>> futures;
    futures.reserve(usingPassNodes.size());
    ResetCommandPool();
    resourceManager.ResetVulkanResource();
    resourceManager.CreateVulkanResource();

    // 为每个 pass 创建 secondary command buffer
    for (auto& t : timeline) {
        for (auto& passIndex : t) {
            auto renderPass = renderPassManager.FindRenderPass(passIndex);
            futures.push_back(threadPool.Enqueue([this, renderPass, passIndex]() -> RenderPassData {
                RenderPassData data;
                data.passIndex = passIndex;
                data.hasDepth = false;
                data.width = vulkanRenderAPI.windowWidth;
                data.height = vulkanRenderAPI.windowHeight;

                // 分配 Secondary Command Buffer
                VkCommandBufferAllocateInfo allocInfo{};
                allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                allocInfo.commandPool = renderPassCommandPools[passIndex][vulkanRenderAPI.currentFrame];
                allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
                allocInfo.commandBufferCount = 1;

                vkAllocateCommandBuffers(vulkanRenderAPI.GetVulkanDevice()->logicalDevice,
                    &allocInfo, &data.secondaryCmd);

                // 收集附件格式信息
                VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;

                auto& resourceCreateIndices = renderPass->GetCreateResources();
                auto& resourceInputIndices = renderPass->GetInputResources();

                // 处理创建的资源
                for(auto& resourceIndex : resourceCreateIndices){
                    auto resource = resourceManager.FindResource(resourceIndex);

                    if(resource && resource->GetType() == ResourceType::Texture){
                        auto description = resource->GetDescription<TextureDescription>();
                        sampleCount = description->samples;
                        data.width = description->width;
                        data.height = description->height;
                        data.arrayLayer = description->arrayLayers;

                        if((description->usages & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) == VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT){
                            data.colorFormats.push_back(description->format);
                            data.colorAttachments.push_back(this->CreateCreateAttachmentInfo(resourceIndex));
                        } else if(description->usages & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT){
                            data.depthFormat = description->format;
                            data.depthAttachment = this->CreateCreateAttachmentInfo(resourceIndex);
                            data.hasDepth = true;
                        }
                    }
                }

                // 处理输入资源
                for(auto& resourceIndex : resourceInputIndices){
                    auto resource = resourceManager.FindResource(resourceIndex);
                    if(resource && resource->GetType() == ResourceType::Texture){
                        auto description = resource->GetDescription<TextureDescription>();
                        if(description->usages & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT){
                            data.colorFormats.push_back(description->format);
                            data.colorAttachments.push_back(this->CreateInputAttachmentInfo(resourceIndex));
                        } else if(description->usages & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT){
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
                inheritanceRenderingInfo.pColorAttachmentFormats = data.colorFormats.empty() ? nullptr : data.colorFormats.data();
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
        }
    }


    int futureIndex = 0;
    for (auto& t : timeline) {
        for (auto& passIndex : t) {
            auto renderPass = renderPassManager.FindRenderPass(passIndex);
            RenderPassData data = futures[futureIndex++].get();
            VkRenderingInfo renderingInfo = {};
            auto& preBarriers = renderPass->GetPreBarriers();
            for (auto& barrier : preBarriers) {
                InsertImageBarrier(commandBuffer, barrier);
            }

            if (!data.colorAttachments.empty() || data.hasDepth) {
                renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
                renderingInfo.pNext = nullptr;
                renderingInfo.flags = VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT; //告訴他在二級緩衝區錄製
                renderingInfo.renderArea.offset = {0, 0};
                renderingInfo.renderArea.extent = {data.width, data.height};
                renderingInfo.layerCount = data.arrayLayer;
                renderingInfo.viewMask = 0;
                renderingInfo.colorAttachmentCount = static_cast<uint32_t>(data.colorAttachments.size());
                renderingInfo.pColorAttachments = data.colorAttachments.empty() ? nullptr : data.colorAttachments.data();
                renderingInfo.pDepthAttachment = data.hasDepth ? &data.depthAttachment : nullptr;
                renderingInfo.pStencilAttachment = nullptr;

                vkCmdBeginRendering(commandBuffer, &renderingInfo);
                vkCmdExecuteCommands(commandBuffer, 1, &data.secondaryCmd);
                vkCmdEndRendering(commandBuffer);
            }

            auto& postBarriers = renderPass->GetPostBarriers();
            for (auto& barrier : postBarriers) {
                InsertImageBarrier(commandBuffer, barrier);
            }
        }
    }

    return *this;
}

//保留因为验证层不会输出在secondary的报错
// FG::FrameGraph& FG::FrameGraph::Execute(const VkCommandBuffer& commandBuffer) {
//     updateBeforeRendering();
//
//     // 注意: 您可能需要检查 ResetCommandPool 的实现。
//     // 如果它专门为二级命令缓冲池设计，现在可能需要调整或暂时禁用。
//     ResetCommandPool();
//
//     resourceManager.ResetVulkanResource();
//     resourceManager.CreateVulkanResource();
//
//     // 串行处理每个pass
//     for (auto& t : timeline) {
//         for (auto& passIndex : t) {
//             auto renderPass = renderPassManager.FindRenderPass(passIndex);
//
//             // --- 数据准备：直接使用局部变量，不再需要 RenderPassData 结构体 ---
//             std::vector<VkRenderingAttachmentInfo> colorAttachments;
//             VkRenderingAttachmentInfo depthAttachment{}; // 使用 {} 初始化
//             bool hasDepth = false;
//             uint32_t width = vulkanRenderAPI.windowWidth;
//             uint32_t height = vulkanRenderAPI.windowHeight;
//             uint32_t arrayLayer = 1;
//
//             // --- 收集附件信息 ---
//             auto& resourceCreateIndices = renderPass->GetCreateResources();
//             auto& resourceInputIndices = renderPass->GetInputResources();
//
//             // 处理创建的资源
//             for(auto& resourceIndex : resourceCreateIndices){
//                 auto resource = resourceManager.FindResource(resourceIndex);
//                 if(resource && resource->GetType() == ResourceType::Texture){
//                     auto description = resource->GetDescription<TextureDescription>();
//                     width = description->width;
//                     height = description->height;
//                     arrayLayer = description->arrayLayers;
//
//                     if((description->usages & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) == VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT){
//                         colorAttachments.push_back(this->CreateCreateAttachmentInfo(resourceIndex));
//                     } else if(description->usages & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT){
//                         depthAttachment = this->CreateCreateAttachmentInfo(resourceIndex);
//                         hasDepth = true;
//                     }
//                 }
//             }
//
//             // 处理输入资源
//             for(auto& resourceIndex : resourceInputIndices){
//                 auto resource = resourceManager.FindResource(resourceIndex);
//                 if(resource && resource->GetType() == ResourceType::Texture){
//                     auto description = resource->GetDescription<TextureDescription>();
//                     if(description->usages & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT){
//                         colorAttachments.push_back(this->CreateInputAttachmentInfo(resourceIndex));
//                     } else if(description->usages & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT){
//                         depthAttachment = this->CreateInputAttachmentInfo(resourceIndex);
//                         hasDepth = true;
//                     }
//                 }
//             }
//
//             // --- 移除了二级命令缓冲区的分配、继承信息设置和 Begin/End 逻辑 ---
//
//             // 1. 插入 Pass 执行前的内存屏障
//             auto& preBarriers = renderPass->GetPreBarriers();
//             for (auto& barrier : preBarriers) {
//                 InsertImageBarrier(commandBuffer, barrier);
//             }
//
//             // 2. 根据有无附件，决定是开始动态渲染还是直接执行命令
//             if (!colorAttachments.empty() || hasDepth) {
//                 // 对于图形 Pass，开始动态渲染
//                 VkRenderingInfo renderingInfo{};
//                 renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
//                 renderingInfo.pNext = nullptr;
//                 // [修改] flags 设置为 0，因为命令直接录制在主命令缓冲区
//                 renderingInfo.flags = 0;
//                 renderingInfo.renderArea.offset = {0, 0};
//                 renderingInfo.renderArea.extent = {width, height};
//                 renderingInfo.layerCount = arrayLayer;
//                 renderingInfo.viewMask = 0;
//                 renderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size());
//                 renderingInfo.pColorAttachments = colorAttachments.empty() ? nullptr : colorAttachments.data();
//                 renderingInfo.pDepthAttachment = hasDepth ? &depthAttachment : nullptr;
//                 renderingInfo.pStencilAttachment = nullptr;
//
//                 vkCmdBeginRendering(commandBuffer, &renderingInfo);
//
//                 // [修改] 直接在主命令缓冲区中设置视口和裁剪矩形
//                 VkViewport viewport{};
//                 viewport.x = 0.0f;
//                 viewport.y = 0.0f;
//                 viewport.width = static_cast<float>(width);
//                 viewport.height = static_cast<float>(height);
//                 viewport.minDepth = 0.0f;
//                 viewport.maxDepth = 1.0f;
//                 vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
//
//                 VkRect2D scissor{};
//                 scissor.offset = {0, 0};
//                 scissor.extent.width = width;
//                 scissor.extent.height = height;
//                 vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
//
//                 // [修改] 直接在主命令缓冲区中执行 Pass 的渲染命令
//                 renderPass->GetExec()(commandBuffer);
//
//                 vkCmdEndRendering(commandBuffer);
//
//                 // [删除] vkCmdExecuteCommands 调用被移除
//
//             } else {
//                 // 对于没有附件的 Pass (例如纯计算或传输)，直接执行其命令
//                 renderPass->GetExec()(commandBuffer);
//             }
//
//             // 3. 插入 Pass 执行后的内存屏障
//             auto& postBarriers = renderPass->GetPostBarriers();
//             for (auto& barrier : postBarriers) {
//                 InsertImageBarrier(commandBuffer, barrier);
//             }
//         }
//     }
//
//     return *this;
// }

FG::FrameGraph & FG::FrameGraph::SetUpdateBeforeRendering(const std::function<void()> &callback) {
    updateBeforeRendering = callback;
    return *this;
}

void FG::FrameGraph::CullPassAndResource() {
    std::unordered_set<uint32_t> usePassNode;
    std::vector<uint32_t> startPassNodes;

    //找到write 或者 proxy 的RenderPass作为反向的起点
    for (auto& renderPassIndex : renderPassNodes) {
        auto renderPassNode = renderPassManager.FindRenderPass(renderPassIndex);
        auto createResources = renderPassNode->GetCreateResources();
        auto outputAttachments = renderPassNode->GetOutputResources();
        if (!createResources.empty() || !outputAttachments.empty()) {
            bool found = false;
            for (auto createResourceIndex : createResources) {
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
            for (auto writeResourceIndex : outputAttachments) {
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
            for (auto& readResourceIndex : readResources) {
                auto readResource = resourceManager.FindResource(readResourceIndex);
                for (auto& renderPassIndex : readResource->GetOutputRenderPass()) {
                    if (!usePassNode.contains(renderPassIndex)) {
                        usePassNode.insert(renderPassIndex);
                        search(renderPassIndex);
                    }
                }
            }
        }
    };

    //开始反向搜索
    for (auto startPassNode : startPassNodes) {
        search(startPassNode);
    }

    //获得工作resource
    for (auto resourceIndex : resourceNodes) {
        auto resource = resourceManager.FindResource(resourceIndex);
        if (resource->isExternal) {
            usingResourceNodes.push_back(resourceIndex);
        } else {
            for (auto& renderPassIndex : resource->GetInputRenderPass()) {
                if (usePassNode.contains(renderPassIndex)) {
                    usingResourceNodes.push_back(resourceIndex);
                }
            }
        }
    }

    //获得工作pass
    for (auto& renderPassNode : renderPassNodes) {
        if (usePassNode.contains(renderPassNode)) {
            usingPassNodes.push_back(renderPassNode);
        }
    }
}

void FG::FrameGraph::CreateTimeline() {
    //创建依赖关系
    std::vector<uint32_t> q;
    std::unordered_set<uint32_t> hasAddedPass;
    for (auto& renderPassIndex : usingPassNodes) {
        auto renderPass = renderPassManager.FindRenderPass(renderPassIndex);
        if (renderPass != nullptr) {
            //清空依赖保证安全
            renderPass->GetRenderPassDependencies().clear();
            for (auto& inputIndex : renderPass->GetInputResources()) {
                auto input = resourceManager.FindResource(inputIndex);
                if (input != nullptr) {
                    for (auto& inputRenderPassIndex : input->GetOutputRenderPass()) {
                        renderPass->AddRenderPassDependency(inputRenderPassIndex);
                    }
                }
            }
            for (auto& readIndex : renderPass->GetReadResources()) {
                auto readResource = resourceManager.FindResource(readIndex);
                if (readResource != nullptr) {
                    for (auto& inputRenderPassIndex : readResource->GetOutputRenderPass()) {
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
    while (!q.empty()) {
        timeline.push_back(q);
        std::vector<uint32_t> tempQ;
        for (auto& renderPassIndex : usingPassNodes) {
            if (hasAddedPass.contains(renderPassIndex)) {
                continue;
            }
            auto renderPass = renderPassManager.FindRenderPass(renderPassIndex);
            if (renderPass != nullptr) {
                auto dependencies = renderPass->GetRenderPassDependencies();
                for (auto& r : q) {
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

    if(hasAddedPass.size() != usingPassNodes.size()) {
        LOG_ERROR("There are some RenderPass have cyclic dependencies !");
        //终止
        return;
    }

    //调整资源在时间线中的使用情况
    for (auto& resourceIndex : usingResourceNodes) {
        auto resource = resourceManager.FindResource(resourceIndex);
        resource->ResetUseTime();
        bool first = false;
        for (int i = 0; i < timeline.size(); ++i) {
            for (auto& renderPassIndex : resource->GetOutputRenderPass()) {
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
            for (auto& renderPassIndex : resource->GetInputRenderPass()) {
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
            LOG_ERROR("the resource : {} don't have input RenderPass", resource->GetName());
            return;
        }
    }
}

void FG::FrameGraph::CreateAliasGroups() {
    std::vector<uint32_t> tempResourceNode;
    for (auto& node : usingResourceNodes) {
        //外部资源不处理
        if (! resourceManager.FindResource(node)->isExternal) {
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
    for (auto& resIndex : tempResourceNode) {
        auto resource = resourceManager.FindResource(resIndex);
        if (resource != nullptr) {
            //首先先将input和output一一对应
            bool foundAlias = false;
            for (auto& renderPassIndex : resource->GetOutputRenderPass()) {
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
                            LOG_ERROR("The RenderPass name : {} input type not equal output type !", renderPass->GetName());
                        }
                        if (! resourceManager.resourceDescriptionToAliasGroup.contains(inputResourceIndex)) {
                            LOG_ERROR("Find the input resource {} didn't create alias than out resource {}",
                                inputResource->GetName(), resource->GetName());
                        }
                        auto& aliasGroup = resourceManager.aliasGroups[resourceManager.resourceDescriptionToAliasGroup[inputResourceIndex]];
                        aliasGroup.sharedResourceIndices.push_back(resIndex);
                        resourceManager.resourceDescriptionToAliasGroup[resIndex] = resourceManager.resourceDescriptionToAliasGroup[inputResourceIndex];
                    }
                }
            }
            if (!foundAlias) {
                auto& aliasGroups = resourceManager.GetAliasGroups();
                for (int i = 0; i < aliasGroups.size(); i++) {
                    if (resourceManager.CanAlias(resIndex, i)) {
                        aliasGroups[i].sharedResourceIndices.push_back(resIndex);
                        resourceManager.resourceDescriptionToAliasGroup[resIndex] = i;
                        foundAlias = true;
                        break;
                    }
                }
            }
            if (!foundAlias) {
                AliasGroup newGroup;
                newGroup.sharedResourceIndices.push_back(resIndex);
                if (resource->GetType() == ResourceType::Texture) {
                    newGroup.description = resource->GetDescription<TextureDescription>();
                }else {
                    newGroup.description = resource->GetDescription<BufferDescription>();
                }
                newGroup.vulkanIndex = -1; //在运行时创建资源
                resourceManager.GetAliasGroups().push_back(newGroup);
                resourceManager.resourceDescriptionToAliasGroup[resIndex] =
                    resourceManager.GetAliasGroups().size() - 1;
            }
        }
    }
}

void FG::FrameGraph::CreateCommandPools(){
    //这里先不考虑compute，计算着色器需要不同queue
    renderPassCommandPools.clear();
    for(auto& renderPassIndex : usingPassNodes){
        renderPassCommandPools[renderPassIndex].resize(MAX_FRAME);
        for (int f = 0; f < MAX_FRAME; f++) {
            VkCommandPoolCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            info.queueFamilyIndex = vulkanRenderAPI.GetVulkanDevice()->queueFamilyIndices.graphics;
            VK_CHECK_RESULT(
                vkCreateCommandPool(vulkanRenderAPI.GetVulkanDevice()->logicalDevice, &info, nullptr,
                                    &renderPassCommandPools[renderPassIndex][f])
            );
        }
    }
}


VkRenderingAttachmentInfo FG::FrameGraph::CreateInputAttachmentInfo(uint32_t resourceIndex){
    auto resourceDesc = resourceManager.FindResource(resourceIndex);
    if(resourceDesc->GetType() == ResourceType::Buffer){
        LOG_ERROR("the resource name: {} is not attachment !", resourceDesc->GetName());
    }

    VkRenderingAttachmentInfo attachmentInfo = {};
    attachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    attachmentInfo.pNext = nullptr; // 重要：明确设置为 nullptr

    auto texture = vulkanRenderAPI.getByIndex<FrameWork::Texture>(resourceManager.GetVulkanResource(resourceIndex));

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

    if((texture->image.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) == VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT){
        attachmentInfo.clearValue.color = vulkanRenderAPI.defaultClearColor;
        attachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }else if((texture->image.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) == VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT){
        attachmentInfo.clearValue.depthStencil = {1.0f, 0};
        attachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    }

    return attachmentInfo;
}

VkRenderingAttachmentInfo FG::FrameGraph::CreateCreateAttachmentInfo(uint32_t resourceIndex){
    VkRenderingAttachmentInfo attachmentInfo = {};
    attachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    attachmentInfo.pNext = nullptr; // 重要：明确设置为 nullptr

    auto texture = vulkanRenderAPI.getByIndex<FrameWork::Texture>(resourceManager.GetVulkanResource(resourceIndex));

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

    if((texture->image.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) == VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT){
        attachmentInfo.clearValue.color = vulkanRenderAPI.defaultClearColor;
        attachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }else if((texture->image.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) == VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT){
        attachmentInfo.clearValue.depthStencil = {1.0f, 0};
        attachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    }

    return attachmentInfo;
}


void FG::FrameGraph::InsertBarriers() {
    struct ResourceState {
        bool readable = false;
        bool writable = false;
    };
    std::unordered_map<uint32_t, ResourceState> resourceStates;
    for (auto& passSet : timeline) {
        for (auto& pass : passSet) {
            auto renderPass = renderPassManager.FindRenderPass(pass);
            //这里先只考虑graphics如果考虑compute 需要再讨论，因为图形管线要求的布局和compute不同, 此处先不考虑Buffer，因为Buffer以Compute Pipeline管理
            if (renderPass != nullptr) {
                for (auto& resourceIndex : renderPass->GetCreateResources()) {
                    auto resource = resourceManager.FindResource(resourceIndex);
                    if (resource) {
                        BarrierInfo newBarrier  = {};
                        newBarrier.resourceID = resourceIndex;
                        //只考虑Image，因为主要原因是布局转换，Buffer不用考虑布局
                        if (resource->GetType() == ResourceType::Texture) {
                            auto description = resource->GetDescription<TextureDescription>();
                            newBarrier.isImage = true;
                            newBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                            newBarrier.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                            newBarrier.srcAccessMask = 0;
                            if ((description->usages & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) == VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
                                newBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                                newBarrier.dstStageMask =  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                                newBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
                            }else if  ((description->usages & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) == VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                                newBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
                                newBarrier.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                                newBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                            }
                            resourceStates[resourceIndex].writable = true;
                            resourceStates[resourceIndex].readable = false;
                            renderPass->AddPreBarrier(newBarrier);

                            if (resource->isPresent) {
                                newBarrier.isImage = true;
                                newBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                                newBarrier.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                                newBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
                                if ((description->usages & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) == VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
                                    newBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                                    newBarrier.dstStageMask =  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                                    newBarrier.dstAccessMask = 0;
                                }
                                resourceStates[resourceIndex].writable = false;
                                resourceStates[resourceIndex].readable = false;
                                renderPass->AddPostBarrier(newBarrier);
                            }
                        }
                    }
                }
                //作为附件输入,此处也不考虑Buffer，如果加入compute Pipeline需要考虑其前驱节点,因为要求的布局不同
                for (auto& resourceIndex : renderPass->GetInputResources()) {
                    auto resource = resourceManager.FindResource(resourceIndex);
                    if(resource->isPresent){
                        LOG_ERROR("The resource name : {} is SwapChain, can't be used as Input Attachment !", resource->GetName());
                    }
                    //资源状态，得知其之前的修改情况
                    auto resourceState = resourceStates[resourceIndex];
                    BarrierInfo newBarrier = {};
                    newBarrier.resourceID = resourceIndex;
                    if (resource->GetType() == ResourceType::Texture) {
                        newBarrier.isImage = true;
                        auto description = resource->GetDescription<TextureDescription>();
                        if (resourceState.readable == false && resourceState.writable == true) {
                            if ((description->usages & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
                                == VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
                                newBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                                newBarrier.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                                newBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
                                newBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                                newBarrier.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                                newBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
                            }else if  ((description->usages & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) == VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                                newBarrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
                                newBarrier.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                                newBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                                newBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
                                newBarrier.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                                newBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                            }
                        }
                        if (resourceState.readable == true && resourceState.writable == false) {
                            if ((description->usages & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
                                == VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
                                newBarrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                                newBarrier.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                                newBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                                newBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                                newBarrier.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                                newBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
                            }else if  ((description->usages & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) == VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT){
                                newBarrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                                newBarrier.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                                newBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                                newBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
                                newBarrier.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                                newBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                            }
                        }
                        resourceStates[resourceIndex].writable = true;
                        resourceStates[resourceIndex].readable = false;
                        renderPass->AddPreBarrier(newBarrier);
                        auto index = (&resourceIndex - renderPass->GetInputResources().data());
                        auto outputResourceIndex = renderPass->GetOutputResources()[index];
                        resourceStates[outputResourceIndex].writable = true;
                        resourceStates[outputResourceIndex].readable = false;
                    }
                }
                for (auto& resourceIndex : renderPass->GetReadResources()) {
                    auto resourceState = &resourceStates[resourceIndex];
                    auto resource = resourceManager.FindResource(resourceIndex);
                    //不需要考虑外部导入的Import Texture， 其在生成时就实现可读（集成再构建Mipmap）
                    //此处先不考虑计算着色器, 所以没有Buffer
                    BarrierInfo newBarrier = {};
                    newBarrier.resourceID = resourceIndex;
                    if (resource) {
                        if(resource->GetType() == ResourceType::Texture){
                            auto description = resource->GetDescription<TextureDescription>();
                            newBarrier.isImage = true;
                            if(resourceState->readable == false && resourceState->writable == true){
                                if ((description->usages & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
                                    == VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
                                    newBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                                    newBarrier.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                                    newBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
                                    newBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                                    newBarrier.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                                    newBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                                }else if  ((description->usages & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) == VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT){
                                    newBarrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
                                    newBarrier.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                                    newBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                                    newBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                                    newBarrier.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                                    newBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                                }
                            }
                            resourceStates[resourceIndex].readable = true;
                            resourceStates[resourceIndex].writable = false;
                            renderPass->AddPreBarrier(newBarrier);
                        }
                    }
                }
            }
        }
    }
}
