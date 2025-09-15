//
// Created by cai on 2025/9/13.
//

#ifndef CAIENGINE_FRAMEGRAPH_H
#define CAIENGINE_FRAMEGRAPH_H
#include<iostream>
#include<vector>

#include "RenderPassManager.h"
#include "ResourceManager.h"

namespace FG {
    class FrameGraph {
    public:
        //依赖注入
        FrameGraph(ResourceManager& resourceManager ,RenderPassManager& renderPassManager);
        //仅仅只是将RenderPass和Resource注入给FrameGraph管理
        FrameGraph& AddResourceNode(uint32_t resourceNode);
        FrameGraph& AddRenderPassNode(uint32_t renderPassNode);
        FrameGraph& Compile();
        FrameGraph& Execute();
        void CullPassAndResource();
        void CreateTimeline();
        void CreateVulkanResources();
        //根据图的拓扑结构创建图的结构
        //为裁剪后的节点创建RenderPass
        void InsertBarriers();
    private:
        std::vector<uint32_t> resourceNodes;
        std::vector<uint32_t> usingResourceNodes;
        std::vector<uint32_t> renderPassNodes;
        std::vector<uint32_t> usingPassNodes; //经过裁剪之后的Pass
        using TimeLine = std::vector<std::vector<uint32_t>>;
        TimeLine timeline;
        ResourceManager& resourceManager;
        RenderPassManager& renderPassManager;
    };
}


#endif //CAIENGINE_FRAMEGRAPH_H