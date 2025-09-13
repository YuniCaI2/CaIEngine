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
        auto writeResources = renderPassNode->GetWriteResources();
        if (createResources.size() > 0 || writeResources.size() > 0) {
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
            for (auto writeResourceIndex : writeResources) {
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
