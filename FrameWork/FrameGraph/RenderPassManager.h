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
    class RenderPass {
    public:
        RenderPass& SetShaderPath(const std::string &shaderPath);

        RenderPass& SetWriteResource(uint32_t index);
        std::unordered_set<uint32_t>& GetWriteResource();
        RenderPass& SetReadResource(uint32_t index);
        std::unordered_set<uint32_t>& GetReadResource();
        RenderPass& SetResolvedWriteResource(uint32_t index);
        std::unordered_set<uint32_t>& GetResolvedWriteResource();

        RenderPass& SetName(const std::string &name);

        RenderPass& CreateVkRenderPass();
    private:
        friend class RenderPassManager;
        std::string name;
        uint32_t shaderID{};
        std::string shaderPath{};
        VkRenderPass renderPass{};
        std::unordered_set<uint32_t> writeResources;
        std::unordered_set<uint32_t> readResources;
        std::unordered_set<uint32_t> resolvedWriteResources; //用于MSAA
        using RenderPassExcFunc = void(std::unique_ptr<RenderPass>& renderPass); //这里不使用std::function 包装器保证性能
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