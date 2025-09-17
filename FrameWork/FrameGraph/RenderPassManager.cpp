//
// Created by cai on 2025/9/13.
//

#include "RenderPassManager.h"



FG::RenderPass & FG::RenderPass::SetCreateResource(uint32_t index) {
    if (std::ranges::find(inputAttachmentResources, index) != std::ranges::end(inputAttachmentResources)) {
        LOG_ERROR("Trying to create resource with index {}, but it has been already in Write", index);
        return *this;
    }
    createResources.push_back(index);
    return *this;
}

std::vector<uint32_t> & FG::RenderPass::GetCreateResources() {
    return createResources;
}

FG::RenderPass & FG::RenderPass::SetInputResource(uint32_t index) {
    inputAttachmentResources.push_back(index);
    return *this;
}

std::vector<uint32_t> & FG::RenderPass::GetInputResources() {
    return inputAttachmentResources;
}

FG::RenderPass & FG::RenderPass::SetOutputResource(uint32_t index) {
    outputAttachmentResources.push_back(index);
    return *this;
}

std::vector<uint32_t> & FG::RenderPass::GetOutputResources() {
    return outputAttachmentResources;
}

uint32_t FG::RenderPassManager::RegisterRenderPass(const std::function<void(std::unique_ptr<RenderPass> &)> &Func) {
    auto renderPass = std::make_unique<RenderPass>();
    Func(renderPass);
    renderPasses.push_back(std::move(renderPass));
    return renderPasses.size() - 1;
}

FG::RenderPass * FG::RenderPassManager::FindRenderPass(const std::string &renderPassName) {
    if (nameToIndex.find(renderPassName) != nameToIndex.end()) {
        return FindRenderPass(nameToIndex[renderPassName]);
    }else {
        LOG_ERROR("RenderPass : {} not found", renderPassName);
        return nullptr;
    }
}

FG::RenderPass * FG::RenderPassManager::FindRenderPass(uint32_t index) {
    if (index >= renderPasses.size()) {
        LOG_ERROR("the index : {} overload renderPasses size", index);
        return nullptr;
    }

    return renderPasses[index].get();
}


FG::RenderPass & FG::RenderPass::SetReadResource(uint32_t index) {
    readResources.push_back(index);
    return *this;
}

std::vector<uint32_t> & FG::RenderPass::GetReadResources() {
    return readResources;
}

FG::RenderPass & FG::RenderPass::AddRenderPassDependency(uint32_t index) {
    renderPassDependencies.insert(index);
    return *this;
}

std::unordered_set<uint32_t> & FG::RenderPass::GetRenderPassDependencies() {
    return renderPassDependencies;
}

FG::RenderPass & FG::RenderPass::AddPreBarrier(const BarrierInfo &barrierInfo) {
    preBarriers.push_back(barrierInfo);
    return *this;
}

FG::RenderPass & FG::RenderPass::AddPostBarrier(const BarrierInfo &barrierInfo) {
    postBarriers.push_back(barrierInfo);
    return *this;
}

std::vector<FG::BarrierInfo> & FG::RenderPass::GetPreBarriers() {
    return preBarriers;
}

std::vector<FG::BarrierInfo> & FG::RenderPass::GetPostBarriers() {
    return postBarriers;
}

FG::RenderPass & FG::RenderPass::SetExec(const std::function<void(VkCommandBuffer)> & Func) {
    renderPassExcFunc = Func;
    return *this;
}

std::function<void(VkCommandBuffer)> & FG::RenderPass::GetExec() {
    return renderPassExcFunc;
}


VkRenderPass& FG::RenderPass::GetVkRenderPass() {
    return renderPass;
}

FG::RenderPass & FG::RenderPass::SetName(const std::string &name) {
    this->name = name;
    return *this;
}

std::string FG::RenderPass::GetName() const {
    return name;
}


