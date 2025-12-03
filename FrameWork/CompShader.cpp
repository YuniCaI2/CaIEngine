//
// Created by cai on 2025/9/23.
//

#include "CompShader.h"
#include "vulkanFrameWork.h"
#include "CompMaterial.h"

FrameWork::CompShader* FrameWork::CompShader::Create(uint32_t& shaderID, const std::string& shaderPath) {
    for (int i = 0; i < compShaderPool.size(); i++) {
        if (compShaderPool[i] == nullptr) {
            shaderID = i;
            compShaderPool[i] = new FrameWork::CompShader(shaderPath);
            return compShaderPool[i];
        }
    }
    compShaderPool.push_back(new FrameWork::CompShader(shaderPath));
    shaderID = compShaderPool.size() - 1;
    return compShaderPool.back();
}

void FrameWork::CompShader::Destroy(const uint32_t &id) {
    if (id >= compShaderPool.size()) {
        LOG_ERROR("CompShader ID : {} out of range", id);
        return ;
    }
    if (compShaderPool[id] != nullptr) {
        delete compShaderPool[id];
    }
    compShaderPool[id] = nullptr;
}

void FrameWork::CompShader::DestroyAll() {
    for (int i = 0; i < compShaderPool.size(); i++) {
        Destroy(i);
    }
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

FrameWork::CompShader *FrameWork::CompShader::Get(const Handle<CompShader> &handle) {
    if (!handle) {
        LOG_ERROR("Trying to access non-existent CompShader handle");
        return nullptr;
    }
    auto shader = compShaderWrappedPool.GetResource(handle.index);
    if (shader == nullptr) {
        LOG_ERROR("CompShader handle {} is nullptr", handle.index);
    }
    return shader;
}

bool FrameWork::CompShader::exist(uint32_t id) {
    if (id >= compShaderPool.size()) {
        LOG_ERROR("CompShader ID : {} out of range", id);
        return false;
    }
    if (compShaderPool[id] == nullptr) {
        LOG_ERROR("CompShader ID : {} is nullptr", id);
        return false;
    }
    return true;
}

FrameWork::CompShader::~CompShader() {
    if (pipelineID != UINT32_MAX) {
        vulkanRenderAPI.DeletePipeline(pipelineID);
    }
}

FrameWork::CompShaderInfo FrameWork::CompShader::GetShaderInfo() const {
    return compShaderInfo;
}

uint32_t FrameWork::CompShader::GetPipelineID() const {
    return pipelineID;
}

void * FrameWork::CompShader::GetShaderPropertyAddress(uint32_t materialDataID, const std::string &name,
    uint32_t id) {
    //先绑定property
    auto materialData = vulkanRenderAPI.getByIndex<FrameWork::CompMaterialData>(materialDataID);
    for (auto& property : compShaderInfo.shaderProperties.baseProperties) {
        if (property.name == name) {
            return reinterpret_cast<char*>
            (materialData->uniformBuffers[vulkanRenderAPI.currentFrame].mapped) +
                property.offset + property.arrayOffset * id;
        }
    }
    LOG_ERROR("PropertyName : {} not found in shader", name);
    return nullptr;
}

void FrameWork::CompShader::Bind(const VkCommandBuffer &cmdBuffer) const {
    auto pipeline = vulkanRenderAPI.getByIndex<FrameWork::VulkanPipeline>(pipelineID)->pipeline;
    vkCmdBindPipeline(
        cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline
        );
}

FrameWork::CompShader::CompShader(const std::string &shaderPath) {
    this->shaderPath = shaderPath;
    compShaderInfo = vulkanRenderAPI.CreateCompPipeline(pipelineID, shaderPath);
}

namespace FrameWork {
    Handle<CompShader> CompShader::CreateHandle(const std::string& shaderPath){
        uint32_t index = compShaderWrappedPool.CreateResource(shaderPath);
        Handle<CompShader> handle;
        handle.index = index;
        return handle;
    }
    bool CompShader::Bind(Handle<CompShader>& handle, const VkCommandBuffer& cmdBuffer){
        if (!handle) {
            LOG_ERROR("Trying to bind a non-existent CompShader handle");
            return false;
        }
        auto shader = compShaderWrappedPool.GetResource(handle.index);
        if (shader) {
            shader->Bind(cmdBuffer);
        } else {
            LOG_ERROR("CaIShader with index {} does not exist", handle.index);
            return false;
        }
        return true;
    }
    CompShaderInfo CompShader::GetInfo(const Handle<CompShader>& shaderHandle){
        return compShaderWrappedPool.GetResource(shaderHandle.index)->GetShaderInfo();
    }
    void* CompShader::GetShaderPropertyAddress(const Handle<CompShader>& shaderHandle, const Handle<CompMaterial>& materialHandle,
        const std::string& name, uint32_t id){
        auto materialData = vulkanRenderAPI.getByIndex<FrameWork::CompMaterialData>(
            CompMaterial::GetMaterialDataID(materialHandle)
        );
        auto compShaderInfo = CompShader::GetInfo(shaderHandle);
        auto shaderPath = CompShader::compShaderWrappedPool.GetResource(shaderHandle.index)->shaderPath;
        //根据name遍历找到对应的地址
        //先绑定property
        for (auto& property : compShaderInfo.shaderProperties.baseProperties) {
            if (property.name == name) {
                return reinterpret_cast<char*>
                (materialData->uniformBuffers[vulkanRenderAPI.currentFrame].mapped) +
                    property.offset + property.arrayOffset * id;
            }
        }
        LOG_ERROR("PropertyName : {} not found in shader", name);
        return nullptr;
        LOG_ERROR("Can't find shader property name : {} in {}", name, shaderPath);
        return nullptr;
    }
    uint32_t CompShader::GetPipelineID(const Handle<CompShader>& pipelineShader){
        return compShaderWrappedPool.GetResource(pipelineShader.index)->pipelineID;
    }
}
