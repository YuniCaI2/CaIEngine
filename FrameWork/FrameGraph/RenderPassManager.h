//
// Created by cai on 2025/9/13.
//

#ifndef CAIENGINE_RENDERPASSMANAGER_H
#define CAIENGINE_RENDERPASSMANAGER_H
#include<iostream>
#include "../vulkanFrameWork.h"

//本FrameGraph现不支持RenderPass的Merge
//这意味着不需要依赖
namespace FG {
    struct BarrierInfo {
        //不考虑family，过于复杂了
        uint32_t resourceID{};
        VkAccessFlags      srcAccessMask{};
        VkAccessFlags      dstAccessMask{};
        VkImageLayout              oldLayout{};
        VkImageLayout              newLayout{};
        VkPipelineStageFlags srcStageMask{};
        VkPipelineStageFlags dstStageMask{};
        bool isImage{};
    };
    enum class PassType {
        Graphics,
        Compute
    };

    class RenderPass {
    public:
        RenderPass& SetCreateResource(uint32_t index);
        std::vector<uint32_t>& GetCreateResources();

        //input和output要求一一对应，简化优化
        std::vector<uint32_t>& GetInputResources();
        std::vector<uint32_t>& GetOutputResources();
        //Input 和 Output 紧耦合防止出错
        RenderPass& SetInputOutputResources(uint32_t input, uint32_t output);

        RenderPass& SetReadResource(uint32_t index);
        std::vector<uint32_t>& GetReadResources();

        RenderPass& AddRenderPassDependency(uint32_t index);
        std::unordered_set<uint32_t>& GetRenderPassDependencies();



        RenderPass& AddPreBarrier(const BarrierInfo& barrierInfo);
        RenderPass& AddPostBarrier(const BarrierInfo& barrierInfo);
        std::vector<BarrierInfo>& GetPreBarriers();
        std::vector<BarrierInfo>& GetPostBarriers();

        void ClearBarriers();

        RenderPass& SetExec(const std::function<void(VkCommandBuffer)>& );
        std::function<void(VkCommandBuffer)>& GetExec();

        RenderPass& SetName(const std::string &name);
        std::string GetName() const;

        VkRenderPass& GetVkRenderPass();
        PassType GetPassType() const;
    private:
        RenderPass& SetInputResource(uint32_t index);
        RenderPass& SetOutputResource(uint32_t index);
        friend class RenderPassManager;
        std::string name;
        PassType passType {PassType::Graphics};//提前知道Pass类型以便与Barrier插入时调整Layout
        VkRenderPass renderPass{};
        std::unordered_set<uint32_t> renderPassDependencies;
        std::vector<uint32_t> createResources;
        std::vector<uint32_t> inputAttachmentResources;
        std::vector<uint32_t> outputAttachmentResources;
        std::vector<uint32_t> readResources;
        std::vector<BarrierInfo> preBarriers;
        std::vector<BarrierInfo> postBarriers;
        using RenderPassExcFunc = std::function<void(VkCommandBuffer)>; //通过捕获
        RenderPassExcFunc renderPassExcFunc;


    };

    class RenderPassManager {
    public:
        uint32_t RegisterRenderPass(const std::function<void(std::unique_ptr<RenderPass>& )>& Func);
        RenderPass* FindRenderPass(const std::string &renderPassName);
        RenderPass* FindRenderPass(uint32_t index);
    private:
        std::unordered_map<std::string, uint32_t> nameToIndex;
        std::vector<std::unique_ptr<RenderPass>> renderPasses;
    };
}


#endif //CAIENGINE_RENDERPASSMANAGER_H