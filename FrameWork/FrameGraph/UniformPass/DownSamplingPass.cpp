//
// Created by 51092 on 2025/9/29.
//

#include "DownSamplingPass.h"
#include "../../CompShader.h"

FG::DownSamplingPass::~DownSamplingPass() {
}

FG::DownSamplingPass::DownSamplingPass(FrameGraph* frameGraph, uint32_t mipmapLevels) {
    this->mipmapLevels = mipmapLevels;
    this->frameGraph = frameGraph;

    //创建外部变量
    FrameWork::CompShader::Create(compShaderID, shaderPath.data());
    //创建纹理
    generateMipAttachments.resize(mipmapLevels - 1);
    generateMipPasses.resize(mipmapLevels - 1);

    for (int i = 0; i < mipmapLevels - 1; i++) {
        generateMipAttachments[i] = frameGraph->GetResourceManager().RegisterResource(
            [&](std::unique_ptr<FG::ResourceDescription>& desc) {
                std::string name = "mipmap" + std::to_string(i);
                desc->SetName(name)
                .SetDescription<FG::TextureDescription>(
                    std::make_unique<FG::TextureDescription>(
                        vulkanRenderAPI.GetFrameWidth() / std::pow(2, i + 1), vulkanRenderAPI.GetFrameHeight() / std::pow(2, i + 1),
                        VK_FORMAT_R16G16B16A16_SFLOAT, mipmapLevels, 1, 1, VK_SAMPLE_COUNT_1_BIT,
                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT
                        )
                    );
            }
            );
    }

}

void FG::DownSamplingPass::Bind() {

}

void FG::DownSamplingPass::SetCreateResource(uint32_t &index) {
    LOG_ERROR("Error: DownSamplingPass can't create Resource");
}

void FG::DownSamplingPass::SetReadResource(const uint32_t &index) {
    LOG_ERROR("Error: DownSamplingPass can't set read Resource");
}

void FG::DownSamplingPass::SetInputOutputResource(const uint32_t &index0, uint32_t &index1) {
    colorAttachment = index0;
    auto colorAtta = frameGraph->GetResourceManager().FindResource(colorAttachment);
    if (colorAtta->GetType() != ResourceType::Texture) {
        LOG_ERROR("Error: DownSamplingPass can't Input Buffer Resource");
        return;
    }
    auto desc = colorAtta->GetDescription<TextureDescription>();
}
