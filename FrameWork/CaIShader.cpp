//
// Created by cai on 2025/9/4.
//

#include "CaIShader.h"
#include"vulkanFrameWork.h"

FrameWork::CaIShader::CaIShader(const std::string &shaderPath, RenderPassType renderPassType) {
    //创建Vulkan资源, 得到ShaderInfo，为了后续创建Material所使用
    shaderInfo = vulkanRenderAPI.CreateVulkanPipeline(pipelineID, shaderPath, renderPassType);
    this->shaderPath = shaderPath;
}

FrameWork::CaIShader::~CaIShader() {
    //释放的方式押入释放队列，因为多飞行帧的原因不能立刻释放资源，释放队列由vulkanRenderAPI管理
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
    ERROR("Can't find shader property name : {}", name);
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
