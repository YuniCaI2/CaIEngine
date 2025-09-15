//
// Created by cai on 2025/9/13.
//

#include "FrameGraph.h"

#include <unordered_set>

#include "ResourceManager.h"

FG::FrameGraph::FrameGraph(ResourceManager &resourceManager, RenderPassManager &renderPassManager):
resourceManager(resourceManager), renderPassManager(renderPassManager)
{}

FG::FrameGraph& FG::FrameGraph::AddResourceNode(uint32_t resourceNode) {
    resourceNodes.push_back(resourceNode);
    return *this;
}

FG::FrameGraph& FG::FrameGraph::AddRenderPassNode(uint32_t renderPassNode) {
    renderPassNodes.push_back(renderPassNode);
    return *this;
}

FG::FrameGraph& FG::FrameGraph::Compile() {
    return *this;
}

FG::FrameGraph& FG::FrameGraph::Execute() {

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
        if (createResources.size() > 0 || outputAttachments.size() > 0) {
            bool found = false;
            for (auto createResourceIndex : createResources) {
                auto resource = resourceManager.FindResource(createResourceIndex);
                if (resource) {
                    if (resource->GetType() == ResourceType::Proxy) {
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
                    if (resource->GetType() == ResourceType::Proxy) {
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
        if (resource->GetType() == ResourceType::Proxy) {
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
        if (!first && resource->GetType() != ResourceType::Proxy) {
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
        if (!last && resource->GetType() != ResourceType::Proxy) {
            LOG_ERROR("the resource : {} don't have input RenderPass", resource->GetName());
            return;
        }
    }
}

void FG::FrameGraph::CreateVulkanResources() {
    std::vector<uint32_t> tempResourceNode;
    for (auto& node : usingResourceNodes) {
        if (resourceManager.FindResource(node)->GetType() != ResourceType::Proxy) {
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
                newGroup.vulkanIndex = resourceManager.CreateVulkanResource(resIndex);
                resourceManager.GetAliasGroups().push_back(newGroup);
                resourceManager.resourceDescriptionToAliasGroup[resIndex] =
                    resourceManager.GetAliasGroups().size() - 1;
            }
        }
    }
}

void FG::FrameGraph::InsertBarriers() {
}
