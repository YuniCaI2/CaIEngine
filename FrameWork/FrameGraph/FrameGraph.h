//
// Created by cai on 2025/9/13.
//

#ifndef CAIENGINE_FRAMEGRAPH_H
#define CAIENGINE_FRAMEGRAPH_H
#include<vector>

#include "ThreadPool.h"
#include "RenderPassManager.h"
#include "ResourceManager.h"

namespace FG {
    class UniformPass;
}

//FrameGraph性能瓶颈在于创建帧间资源，这是重开销
//后续可以将RenderPass进行合并，比如可以将相邻的Pass如果管线一致可以防止其重复绑定,所以需要加一个数组记录管线？
namespace FG {
    class FrameGraph {
    public:
        //依赖注入
        FrameGraph(ResourceManager& resourceManager ,RenderPassManager& renderPassManager);
        ~FrameGraph();
        //仅仅只是将RenderPass和Resource注入给FrameGraph管理
        FrameGraph& AddResourceNode(uint32_t resourceNode);
        FrameGraph& AddRenderPassNode(uint32_t renderPassNode);
        FrameGraph& Compile();
        FrameGraph& Execute(const VkCommandBuffer& commandBuffer);
        FrameGraph& SetUpdateBeforeRendering(const std::function<void()>& callback);
        void CullPassAndResource();
        void CreateTimeline();
        void CreateAliasGroups();
        void CreateCommandPools();
        VkRenderingAttachmentInfo CreateCreateAttachmentInfo(uint32_t resourceIndex);
        VkRenderingAttachmentInfo CreateInputAttachmentInfo(uint32_t resourceIndex);

        ResourceManager& GetResourceManager();
        RenderPassManager& GetRenderPassManager();
        //根据图的拓扑结构创建图的结构
        //为裁剪后的节点创建RenderPass
    private:
        void InsertBarriers2();
        void InsertImageBarrier(VkCommandBuffer cmdBuffer, const BarrierInfo& barrier);
        std::vector<uint32_t> resourceNodes{};
        std::vector<uint32_t> usingResourceNodes{};
        std::vector<uint32_t> renderPassNodes{};
        std::vector<uint32_t> usingPassNodes{}; //经过裁剪之后的Pass
        using TimeLine = std::vector<std::vector<uint32_t>>;
        TimeLine timeline;
        ResourceManager& resourceManager;
        RenderPassManager& renderPassManager;
        ThreadPool threadPool;
        std::function<void()> updateBeforeRendering{};

        //单FrameGraph资源
        std::unordered_map<uint32_t, VkCommandPool> renderPassCommandPools; //每个Pass对应一个commandPool来创建子command，多线程录制
        std::unordered_map<uint32_t, std::vector<VkCommandBuffer>> commandBuffers; //这里是每个飞行帧一个commandBuffer
    };
}


#endif //CAIENGINE_FRAMEGRAPH_H