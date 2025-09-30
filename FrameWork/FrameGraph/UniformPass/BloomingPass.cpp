//
// Created by 51092 on 2025/9/30.
//

#include "BloomingPass.h"
#include "../../CompShader.h"
#include "../../CompMaterial.h"

FG::BloomingPass::~BloomingPass() {
    for (int i = 0; i < mipmapLevels; i++) {
        FrameWork::CompMaterial::Destroy(compRowMaterials[i]);
        FrameWork::CompMaterial::Destroy(compColMaterials[i]);
    }
    FrameWork::CompShader::Destroy(colShaderID);
    FrameWork::CompShader::Destroy(rowShaderID);
}

FG::BloomingPass::BloomingPass(FrameGraph *frameGraph, uint32_t mipmapLevels): downSampling(frameGraph, mipmapLevels) {
    this->mipmapLevels = mipmapLevels;
    this->frameGraph = frameGraph;

    compColMaterials.resize(mipmapLevels - 1);
    compRowMaterials.resize(mipmapLevels - 1);

    //创建管线资源
    FrameWork::CompShader::Create(rowShaderID, rowShaderPath.data());
    FrameWork::CompShader::Create(colShaderID, colShaderPath.data());
    FrameWork::CompShader::Create(specShaderID, specShaderPath.data());
    FrameWork::CompShader::Create(blendShaderID, blendShaderPath.data());
    for (int i = 0; i < mipmapLevels - 1; i++) {
        FrameWork::CompMaterial::Create(compColMaterials[i], colShaderID);
        FrameWork::CompMaterial::Create(compRowMaterials[i], rowShaderID);
    }
    FrameWork::CompMaterial::Create(getSpecMaterialID, specShaderID);
    FrameWork::CompMaterial::Create(blendMaterialID, blendShaderID);

}

void FG::BloomingPass::Bind() {
    downSampling.Bind();
}

void FG::BloomingPass::SetCreateResource(uint32_t &index) {
    LOG_ERROR("Error: BloomingPass can't create Resource");
}

void FG::BloomingPass::SetReadResource(const uint32_t &index) {
    LOG_ERROR("Error: BloomingPass can't set read Resource");

}

void FG::BloomingPass::SetInputOutputResource(const uint32_t &index0, uint32_t &index1) {
    colorAttachment = index0;
    auto& resourceManager = frameGraph->GetResourceManager();
    auto& renderPassManager = frameGraph->GetRenderPassManager();
    auto desc = frameGraph->GetResourceManager().FindResource(colorAttachment);
    if (desc->GetType() != ResourceType::Texture) {
        LOG_ERROR("Error: BloomingPass color Attachment: {} type is texture Resource", colorAttachment);
        return;
    }
    auto texDesc = desc->GetDescription<TextureDescription>();

    specAttachment = resourceManager.RegisterResource(
        [&](std::unique_ptr<ResourceDescription>& Desc) {
            Desc->SetName("SpecAttachment")
            .SetDescription<TextureDescription>(
            std::make_unique<FG::TextureDescription>(
                 texDesc->width, texDesc->height,
                texDesc->format, mipmapLevels, 1, 1, texDesc->samples, //一般接受的是一个resolve,采样保证为1
                texDesc->usages
                )
                );
        }
        );

    specPass = renderPassManager.RegisterRenderPass(
        [&](std::unique_ptr<RenderPass>& renderPass) {
            renderPass->SetName("SpecRenderPass")
            .SetPassType(PassType::Compute)
            .SetExec(
                [&](VkCommandBuffer cmdBuffer) {
                    uint32_t width = texDesc->width;
                    uint32_t height = texDesc->height;
                    FrameWork::CompShader::Get(specShaderID)->Bind(cmdBuffer);
                    auto material = FrameWork::CompMaterial::Get(getSpecMaterialID);
                    material->SetAttachment(
                        "srcImage", resourceManager.GetVulkanIndex(colorAttachment)
                        );
                    material->SetStorageImage2D(
                        "dstImage", resourceManager.GetVulkanIndex(specAttachment)
                        );
                    material->SetParam("threshold", 1.0f);
                    material->SetParam("dstScale", glm::vec2(width, height));
                    material->SetParam("invDstScale", glm::vec2(1.0f / height, 1.0f / width));
                    material->Bind(cmdBuffer);
                    vkCmdDispatch(cmdBuffer, (width + 15) / 16,
                        (height + 15) / 16, 1);
                });
        });
    renderPassManager.FindRenderPass(specPass)->SetReadResource(colorAttachment).SetCreateResource(specAttachment);
    frameGraph->AddResourceNode(colorAttachment).AddRenderPassNode(specAttachment).AddResourceNode(specAttachment);

    //降采样
    downSampling.SetInputOutputResource(specAttachment, downSamplingAttachment);

    //升采样

}
