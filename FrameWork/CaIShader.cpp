//
// Created by cai on 2025/9/4.
//

#include "CaIShader.h"
#include"vulkanFrameWork.h"
FrameWork::CaIShader* FrameWork::CaIShader::Create(uint32_t &id, const std::string& shaderPath, RenderPassType renderPassType) {
    for (int i = 0; i < caiShaderPool.size(); i++) {
        if (caiShaderPool[i] == nullptr) {
            id = i;
            return new FrameWork::CaIShader(shaderPath, renderPassType);
        }
    }
    caiShaderPool.push_back(new FrameWork::CaIShader(shaderPath, renderPassType));
    id = caiShaderPool.size() - 1;
    return caiShaderPool.back();
}


void FrameWork::CaIShader::Destroy(uint32_t &id) {
    if (id < caiShaderPool.size() && caiShaderPool[id] != nullptr) {
        delete caiShaderPool[id];
        caiShaderPool[id] = nullptr;
        return;
    }
    LOG_WARNING("Destroy {} is not existed shader", id);
}

FrameWork::CaIShader * FrameWork::CaIShader::Get(uint32_t id) {
    if (id < caiShaderPool.size() && caiShaderPool[id] != nullptr) {
        return caiShaderPool[id];
    }
    LOG_ERROR("Trying to access non-existent shader id {}", id);
    return nullptr;
}

void FrameWork::CaIShader::DestroyAll() {
    for (auto& shader: caiShaderPool) {
        delete shader;
    }
}

bool FrameWork::CaIShader::exist(uint32_t id) {
    if (id < caiShaderPool.size() && caiShaderPool[id] != nullptr) {
        return true;
    }
    return false;
}

FrameWork::CaIShader::CaIShader(const std::string &shaderPath, RenderPassType renderPassType) {
    //创建Vulkan资源, 得到ShaderInfo，为了后续创建Material所使用
    shaderInfo = vulkanRenderAPI.CreateVulkanPipeline(pipelineID, shaderPath, renderPassType);
    this->shaderPath = shaderPath;
}

FrameWork::CaIShader::CaIShader(const std::string &shaderPath, VkRenderPass renderPass) {
    shaderInfo = vulkanRenderAPI.CreateVulkanPipeline(pipelineID, shaderPath, renderPass);
    this->shaderPath = shaderPath;
}

//支持Dynamic Rendering
FrameWork::CaIShader::CaIShader(const std::string &shaderPath) {
    shaderInfo = vulkanRenderAPI.CreateVulkanPipeline(pipelineID, shaderPath);
    this->shaderPath = shaderPath;
}

FrameWork::CaIShader::~CaIShader() {
    //释放的方式押入释放队列，因为多飞行帧的原因不能立刻释放资源，释放队列由vulkanRenderAPI管理
    if (pipelineID != UINT32_MAX)
        vulkanRenderAPI.DeletePipeline(pipelineID);
}

void * FrameWork::CaIShader::GetShaderPropertyAddress(uint32_t materialDataID, const std::string &name, uint32_t id) {
    auto materialData = vulkanRenderAPI.getByIndex<FrameWork::MaterialData>(materialDataID);
    //根据name遍历找到对应的地址
    for (auto& property :  shaderInfo.vertProperties.baseProperties) {
        if (property.name == name) {
            return reinterpret_cast<char*>
            (materialData->vertexUniformBuffers[vulkanRenderAPI.currentFrame].mapped) + property.offset + property.arrayOffset * id;
        }
    }
    for (auto& property : shaderInfo.fragProperties.baseProperties) {
        if (property.name == name) {
            return reinterpret_cast<char*>
            (materialData->fragmentUniformBuffers[vulkanRenderAPI.currentFrame].mapped) + property.offset + property.arrayOffset * id;
        }
    }
    LOG_ERROR("Can't find shader property name : {}", name);
    return nullptr;
}

void FrameWork::CaIShader::Bind(const VkCommandBuffer &cmdBuffer) const {
    auto pipeline = vulkanRenderAPI.getByIndex<FrameWork::VulkanPipeline>(pipelineID)->pipeline;
    vkCmdBindPipeline(
        cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline
        );
}

FrameWork::ShaderInfo FrameWork::CaIShader::GetShaderInfo() const {
    return shaderInfo;
}

uint32_t FrameWork::CaIShader::GetPipelineID() const {
    return pipelineID;
}
