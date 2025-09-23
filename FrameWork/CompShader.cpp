//
// Created by cai on 2025/9/23.
//

#include "CompShader.h"

uint32_t FrameWork::CompShader::Create(const std::string& shaderPath) {

    return {};
}

void FrameWork::CompShader::Destroy(uint32_t &id) {
    if (id >= compShaderPool.size()) {
        LOG_ERROR("CompShader ID : {} out of range", id);
        return ;
    }
    if (compShaderPool[id] != nullptr) {
        delete compShaderPool[id];
    }
    compShaderPool[id] = nullptr;
}

FrameWork::CompShader * FrameWork::CompShader::Get(uint32_t id) {
    if (id >= compShaderPool.size()) {
        LOG_ERROR("CompShader ID : {} out of range", id);
        return nullptr;
    }
    if (compShaderPool[id] == nullptr) {
        LOG_ERROR("CompShader ID : {} is nullptr", id);
    }
    return compShaderPool[id];
}

FrameWork::CompShaderInfo FrameWork::CompShader::GetShaderInfo() const {
    return {};
}

uint32_t FrameWork::CompShader::GetPipelineID() const {
    return pipelineID;
}

FrameWork::CompShader::CompShader(const std::string &shaderPath) {
    this->shaderPath = shaderPath;
}
