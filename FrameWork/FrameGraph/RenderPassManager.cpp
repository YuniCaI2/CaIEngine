//
// Created by cai on 2025/9/13.
//

#include "RenderPassManager.h"

FG::RenderPass & FG::RenderPass::SetShaderPath(const std::string &shaderPath) {
    this->shaderPath = shaderPath;
    return *this;
}

FG::RenderPass & FG::RenderPass::SetCreateResource(uint32_t index) {
    createResources.insert(index);
    return *this;
}

std::unordered_set<uint32_t> & FG::RenderPass::GetCreateResources() {
    return createResources;
}

FG::RenderPass & FG::RenderPass::CreateVkRenderPass() {
    return *this;
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

FG::RenderPass & FG::RenderPass::SetWriteResource(uint32_t index) {
    writeResources.insert(index);
    return *this;
}

std::unordered_set<uint32_t> & FG::RenderPass::GetWriteResources() {
    return writeResources;
}

FG::RenderPass & FG::RenderPass::SetReadResource(uint32_t index) {
    readResources.insert(index);
    return *this;
}

std::unordered_set<uint32_t> & FG::RenderPass::GetReadResources() {
    return readResources;
}

FG::RenderPass & FG::RenderPass::SetResolvedWriteResource(uint32_t index) {
    resolvedWriteResources.insert(index);
    return *this;
}

std::unordered_set<uint32_t> & FG::RenderPass::GetResolvedWriteResource() {
    return resolvedWriteResources;
}

FG::RenderPass & FG::RenderPass::SetName(const std::string &name) {
    this->name = name;
    return *this;
}


