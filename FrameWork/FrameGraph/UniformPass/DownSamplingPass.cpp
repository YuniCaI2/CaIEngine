//
// Created by 51092 on 2025/9/29.
//

#include "DownSamplingPass.h"
#include "../../CompShader.h"
#include "../../CompMaterial.h"

FG::DownSamplingPass::~DownSamplingPass() {
    for (int i = 0; i < mipmapLevels - 1; i++) {
        FrameWork::CompMaterial::Destroy(compMaterials[i]);
    }
    FrameWork::CompShader::Destroy(compShaderID);
    //释放等后面动态节点的时候考虑
}

FG::DownSamplingPass::DownSamplingPass(FrameGraph* frameGraph, uint32_t mipmapLevels) {
    this->mipmapLevels = mipmapLevels;
    this->frameGraph = frameGraph;

    compMaterials.resize(mipmapLevels - 1);
    //创建外部变量
    FrameWork::CompShader::Create(compShaderID, shaderPath.data());
    for (int i = 0; i < mipmapLevels - 1; i++) {
        FrameWork::CompMaterial::Create(compMaterials[i], compShaderID);
    }
    //创建纹理
    generateMipAttachments.resize(mipmapLevels - 1);
    generateMipPasses.resize(mipmapLevels - 1);


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
    auto texDesc = colorAtta->GetDescription<TextureDescription>();
    if (texDesc->samples != VK_SAMPLE_COUNT_1_BIT) {
        LOG_ERROR("In DownSampling Pass, samples couldn't be bigger than 1");
    }
    if (texDesc->mipLevels != mipmapLevels) {
        LOG_ERROR("In DownSampling, Input texDesc mipmap is not equal DownSampling mipmap");
    }
    for (int i = 0; i < mipmapLevels - 1; i++) {
        generateMipAttachments[i] = frameGraph->GetResourceManager().RegisterResource(
            [&](std::unique_ptr<FG::ResourceDescription>& desc) {
                std::string name = "mipmap" + std::to_string(i);
                desc->SetName(name)
                .SetDescription<FG::TextureDescription>(
                    std::make_unique<FG::TextureDescription>(
                         texDesc->width, texDesc->height,
                        texDesc->format, mipmapLevels, 1, 1, texDesc->samples, //一般接受的是一个resolve
                        texDesc->usages
                        )
                    );
            }
            );

        generateMipPasses[i] = frameGraph->GetRenderPassManager().RegisterRenderPass(
            [&](std::unique_ptr<FG::RenderPass>& renderPass) {
                std::string name = "generateMipmapRenderPass" + std::to_string(i);

                renderPass->SetName(name).SetPassType(FG::PassType::Compute)
                .SetExec([&, i](VkCommandBuffer cmdBuffer) {
                    auto compMaterial = FrameWork::CompMaterial::Get(compMaterials[i]);
                    auto desc = frameGraph->GetResourceManager().FindResource(generateMipAttachments[i])->GetDescription<FG::TextureDescription>();
                    uint32_t width, height;
                    width = desc->width / std::pow(2, i + 1);
                    height = desc->height / std::pow(2, i + 1);
                    FrameWork::CompShader::Get(compShaderID)->Bind(cmdBuffer);
                    if (i != 0) {
                        compMaterial->SetAttachment(
                            "srcImage", frameGraph->GetResourceManager().GetVulkanIndex(generateMipAttachments[i - 1]));
                    }else {
                        compMaterial->SetAttachment(
                            "srcImage", frameGraph->GetResourceManager().GetVulkanIndex(colorAttachment));
                    }
                    compMaterial->SetStorageImage2D(
                        "dstImage", frameGraph->GetResourceManager().GetVulkanIndex(generateMipAttachments[i]), i + 1);
                    compMaterial->SetParam("srcLod", i);
                    compMaterial->SetParam("dstScale", glm::vec2(width, height));
                    compMaterial->SetParam("invDstScale", glm::vec2(1.0f / width, 1.0f / height));
                    compMaterial->Bind(cmdBuffer);
                    vkCmdDispatch(cmdBuffer, (width + 15) / 16,
                        (height + 15) / 16, 1);
                });
            });
    }
    frameGraph->AddResourceNode(colorAttachment);
    for (int i = 0; i < mipmapLevels - 1; i++) {
        frameGraph->AddResourceNode(generateMipAttachments[i]).AddRenderPassNode(generateMipPasses[i]);
        if (i == 0) {
            frameGraph->GetRenderPassManager().FindRenderPass(generateMipPasses[i])->SetInputOutputResources(colorAttachment,
                generateMipAttachments[i]);
        }else {
            frameGraph->GetRenderPassManager().FindRenderPass(generateMipPasses[i])->SetInputOutputResources(generateMipAttachments[i - 1],
                generateMipAttachments[i]);
        }
    }
    index1 = generateMipAttachments.back();
}