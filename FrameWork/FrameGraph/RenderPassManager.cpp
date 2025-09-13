//
// Created by cai on 2025/9/13.
//

#include "RenderPassManager.h"

FG::RenderPass & FG::RenderPass::SetShaderPath(const std::string &shaderPath) {
    this->shaderPath = shaderPath;
}

FG::RenderPass & FG::RenderPass::SetWriteResource(uint32_t index) {
    writeResources.insert(index);
}

std::unordered_set<uint32_t> & FG::RenderPass::GetWriteResource() {
    return writeResources;
}

FG::RenderPass & FG::RenderPass::SetReadResource(uint32_t index) {
    readResources.insert(index);
}

std::unordered_set<uint32_t> & FG::RenderPass::GetReadResource() {
    return readResources;
}

FG::RenderPass & FG::RenderPass::SetResolvedWriteResource(uint32_t index) {
    resolvedWriteResources.insert(index);
}

std::unordered_set<uint32_t> & FG::RenderPass::GetResolvedWriteResource() {
    return resolvedWriteResources;
}

FG::RenderPass & FG::RenderPass::SetName(const std::string &name) {
    this->name = name;
}


