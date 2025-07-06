//
// Created by 51092 on 25-6-13.
//

#include "Resource.h"

#include "VulkanTool.h"

FrameWork::Resource::Resource() {
}

VkShaderModule FrameWork::Resource::getShaderModulFromFile(VkDevice device, const std::string &fileName, VkShaderStageFlags shaderStage) const{
    auto shaderPath = generalShaderPath + fileName + "/" + fileName;
    switch (shaderStage) {
        case VK_SHADER_STAGE_VERTEX_BIT:
            shaderPath += ".vert";
            break;
        case VK_SHADER_STAGE_FRAGMENT_BIT:
            shaderPath += ".frag";
            break;
        case VK_SHADER_STAGE_COMPUTE_BIT:
            shaderPath += ".comp";
            break;
        default:
            break;
    }
    shaderPath += ".spv";

    return VulkanTool::loadShader(shaderPath, device);
}
