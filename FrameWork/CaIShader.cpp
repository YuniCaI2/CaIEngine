//
// Created by cai on 2025/9/4.
//

#include "CaIShader.h"

FrameWork::CaIShader::CaIShader(const std::string &shaderPath, RenderPassType renderPassType) {
    //创建Vulkan资源, 得到ShaderInfo，为了后续创建Material所使用
    shaderInfo = vulkanRenderAPI.CreateVulkanPipeline(pipelineID, shaderPath, renderPassType);
    this->shaderPath = shaderPath;
}

FrameWork::CaIShader::~CaIShader() {
    //释放的方式押入释放队列，因为多飞行帧的原因不能立刻释放资源，释放队列由vulkanRenderAPI管理
}
