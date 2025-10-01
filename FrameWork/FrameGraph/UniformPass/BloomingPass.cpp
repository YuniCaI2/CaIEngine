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

FG::BloomingPass::BloomingPass(FrameGraph *frameGraph, uint32_t mipmapLevels, float*  thres): downSampling(frameGraph, mipmapLevels) {
    this->mipmapLevels = mipmapLevels;
    this->frameGraph = frameGraph;
    this->threshold = thres;

    compColMaterials.resize(mipmapLevels);
    compRowMaterials.resize(mipmapLevels);

    //创建管线资源
    FrameWork::CompShader::Create(rowShaderID, rowShaderPath.data());
    FrameWork::CompShader::Create(colShaderID, colShaderPath.data());
    FrameWork::CompShader::Create(specShaderID, specShaderPath.data());
    FrameWork::CompShader::Create(blendShaderID, blendShaderPath.data());
    for (int i = 0; i < mipmapLevels; i++) {
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

    generateRowAttachment = resourceManager.RegisterResource(
        [&](std::unique_ptr<ResourceDescription>& Desc) {
            Desc->SetName("generateRowAttachment")
            .SetDescription<TextureDescription>(
            std::make_unique<FG::TextureDescription>(
                 texDesc->width, texDesc->height,
                texDesc->format, mipmapLevels, 1, 1, texDesc->samples, //一般接受的是一个resolve,采样保证为1
                texDesc->usages
                )
                );
        }
        );

    generateColAttachment = resourceManager.RegisterResource(
        [&](std::unique_ptr<ResourceDescription>& Desc) {
            Desc->SetName("generateColAttachment")
            .SetDescription<TextureDescription>(
            std::make_unique<FG::TextureDescription>(
                 texDesc->width, texDesc->height,
                texDesc->format, mipmapLevels, 1, 1, texDesc->samples, //一般接受的是一个resolve,采样保证为1
                texDesc->usages
                )
                );
        }
        );

    generateColAttachments.resize(mipmapLevels);
    for (int i = 0; i < mipmapLevels; i++) {
        generateColAttachments[i] = resourceManager.RegisterResource(
            [&](std::unique_ptr<ResourceDescription>& Desc) {
                Desc->SetName("generateColAttachment_" + std::to_string(i))
                .SetDescription<TextureDescription>(
                std::make_unique<FG::TextureDescription>(
                     texDesc->width, texDesc->height,
                    texDesc->format, mipmapLevels, 1, 1, texDesc->samples, //一般接受的是一个resolve,采样保证为1
                    texDesc->usages
                    )
                    );
        });
    }


    blendAttachment = resourceManager.RegisterResource(
        [&](std::unique_ptr<ResourceDescription>& Desc) {
            Desc->SetName("blendAttachment")
            .SetDescription<TextureDescription>(
            std::make_unique<FG::TextureDescription>(
                 texDesc->width, texDesc->height,
                texDesc->format, texDesc->mipLevels, texDesc->resolveMipLevels, texDesc->arrayLayers, texDesc->samples, //和colorAttachment一致,作为colorAttachment在FrameGraph的替身
                texDesc->usages
                )
                );
        }
        );

    specPass = renderPassManager.RegisterRenderPass(
        [&, texDesc](std::unique_ptr<RenderPass>& renderPass) {
            renderPass->SetName("SpecRenderPass")
            .SetPassType(PassType::Compute)
            .SetExec(
                [&, texDesc](VkCommandBuffer cmdBuffer) {
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
                    float thres = threshold ? *threshold : 1.0f;
                    material->SetParam("threshold", thres);
                    material->SetParam("dstScale", glm::vec2(width, height));
                    material->SetParam("invDstScale", glm::vec2(1.0f / width, 1.0f / height));
                    material->Bind(cmdBuffer);
                    vkCmdDispatch(cmdBuffer, (width + 15) / 16,
                        (height + 15) / 16, 1);
                });
        });
    renderPassManager.FindRenderPass(specPass)->SetReadResource(colorAttachment).SetCreateResource(specAttachment);
    frameGraph->AddResourceNode(colorAttachment).AddRenderPassNode(specPass).AddResourceNode(specAttachment);

    //降采样
    downSampling.SetInputOutputResource(specAttachment, downSamplingAttachment);

    //升采样

    upRowPass = renderPassManager.RegisterRenderPass(
        [&](std::unique_ptr<RenderPass>& renderPass) {
            renderPass->SetName("upRowPass")
            .SetPassType(PassType::Compute)
            .SetExec([&, texDesc](VkCommandBuffer cmdBuffer) {
                for (int i = mipmapLevels - 1; i >= 0; i--) {
                    uint32_t width = (double)texDesc->width / std::pow(2, i);
                    uint32_t height = (double)texDesc->height / std::pow(2, i);
                    auto rowMaterial = FrameWork::CompMaterial::Get(compRowMaterials[i]);
                    rowMaterial->SetStorageImage2D("dstImage", resourceManager.GetVulkanIndex(generateRowAttachment), i);
                    rowMaterial->SetTexture("srcImage", resourceManager.GetVulkanIndex(downSamplingAttachment));
                    rowMaterial->SetParam("dstLod", static_cast<float>(i));
                    rowMaterial->SetParam("dstScale", glm::vec2(width, height));
                    rowMaterial->SetParam("invDstScale", glm::vec2(1.0f / width, 1.0f / height));
                    FrameWork::CompShader::Get(rowShaderID)->Bind(cmdBuffer);
                    rowMaterial->Bind(cmdBuffer);
                    vkCmdDispatch(cmdBuffer, (width + 15) / 16,
                    (height + 15) / 16, 1);
                }
                });
        }
        );
    renderPassManager.FindRenderPass(upRowPass)->SetReadResource(downSamplingAttachment).SetCreateResource(generateRowAttachment);
    frameGraph->AddRenderPassNode(upRowPass).AddResourceNode(generateRowAttachment);

    upSamplingPass.resize(mipmapLevels);
    for (int i = mipmapLevels - 1; i >= 0; i--) {
        upSamplingPass[i] = renderPassManager.RegisterRenderPass(
            [&](std::unique_ptr<RenderPass>& renderPass) {
                std::string name = "UpSamplingRenderPass_" + std::to_string(i);
                renderPass->SetName(name).SetPassType(PassType::Compute)
                .SetExec([&, i, texDesc](VkCommandBuffer cmdBuffer) {
                    uint32_t width = (double)texDesc->width / std::pow(2, i);
                    uint32_t height = (double)texDesc->height / std::pow(2, i);
                    auto colMaterial = FrameWork::CompMaterial::Get(compColMaterials[i]);
                    colMaterial->SetStorageImage2D("dstImage", resourceManager.GetVulkanIndex(generateColAttachments[i]), i);
                    colMaterial->SetTexture("srcImage", resourceManager.GetVulkanIndex(generateRowAttachment));
                    colMaterial->SetTexture("blendImage", resourceManager.GetVulkanIndex(generateColAttachments[i]));
                    colMaterial->SetParam("maxLod", static_cast<float>(mipmapLevels - 1));
                    colMaterial->SetParam("dstLod", static_cast<float>(i));
                    colMaterial->SetParam("dstScale", glm::vec2(width, height));
                    colMaterial->SetParam("invDstScale", glm::vec2(1.0f / width, 1.0f / height));
                    FrameWork::CompShader::Get(colShaderID)->Bind(cmdBuffer);
                    colMaterial->Bind(cmdBuffer);
                    vkCmdDispatch(cmdBuffer, (width + 15) / 16,
                    (height + 15) / 16, 1);
                });
            });

        //实际上colAttachments所对应的物理资源相同的不同FrameGraph节点，目的是防止成环
        if (i == mipmapLevels - 1) {
            renderPassManager.FindRenderPass(upSamplingPass[i])->SetReadResource(generateRowAttachment).SetCreateResource(generateColAttachments[i]);
            frameGraph->AddRenderPassNode(upSamplingPass[i]).AddResourceNode(generateColAttachments[i]);
        }else {
            renderPassManager.FindRenderPass(upSamplingPass[i])->SetReadResource(generateRowAttachment).
            SetInputOutputResources(generateColAttachments[i + 1] ,generateColAttachments[i]);
            frameGraph->AddRenderPassNode(upSamplingPass[i]).AddResourceNode(generateColAttachments[i]);
        }
    }

    // upColPass = renderPassManager.RegisterRenderPass(
    //     [&, texDesc](std::unique_ptr<RenderPass>& renderPass) {
    //         renderPass->SetName("upColPass")
    //         .SetPassType(PassType::Compute)
    //         .SetExec([&, texDesc](VkCommandBuffer cmdBuffer) {
    //             for (int i = mipmapLevels - 1; i >= 0; i--) {
    //                 uint32_t width = (double)texDesc->width / std::pow(2, i);
    //                 uint32_t height = (double)texDesc->height / std::pow(2, i);
    //                 auto colMaterial = FrameWork::CompMaterial::Get(compColMaterials[i]);
    //                 colMaterial->SetStorageImage2D("dstImage", resourceManager.GetVulkanIndex(generateColAttachment), i);
    //                 colMaterial->SetTexture("srcImage", resourceManager.GetVulkanIndex(generateRowAttachment));
    //                 colMaterial->SetTexture("blendImage", resourceManager.GetVulkanIndex(generateColAttachment));
    //                 colMaterial->SetParam("maxLod", mipmapLevels - 1);
    //                 colMaterial->SetParam("dstLod", static_cast<float>(i));
    //                 colMaterial->SetParam("dstScale", glm::vec2(width, height));
    //                 colMaterial->SetParam("invDstScale", glm::vec2(1.0f / width, 1.0f / height));
    //                 FrameWork::CompShader::Get(colShaderID)->Bind(cmdBuffer);
    //                 colMaterial->Bind(cmdBuffer);
    //                 vkCmdDispatch(cmdBuffer, (width + 15) / 16,
    //                 (height + 15) / 16, 1);
    //             }
    //             });
    //     }
    //     );
    // renderPassManager.FindRenderPass(upColPass)->SetReadResource(generateRowAttachment).SetCreateResource(generateColAttachment);
    // frameGraph->AddRenderPassNode(upColPass).AddResourceNode(generateColAttachment);

    //Blend
    blendPass = renderPassManager.RegisterRenderPass(
        [&](std::unique_ptr<RenderPass>& renderPass) {
            renderPass->SetName("blendPass")
            .SetPassType(PassType::Compute)
            .SetExec([&, texDesc](VkCommandBuffer cmdBuffer) {
                uint32_t width = texDesc->width;
                uint32_t height = texDesc->height;
                auto material = FrameWork::CompMaterial::Get(blendMaterialID);
                material->SetTexture("srcImage", resourceManager.GetVulkanIndex(generateColAttachments[0]));
                material->SetStorageImage2D("dstImage", resourceManager.GetVulkanIndex(colorAttachment), 0);
                material->SetParam("dstScale", glm::vec2(width, height));
                material->SetParam("invDstScale", glm::vec2(1.0f / width, 1.0f / height));
                FrameWork::CompShader::Get(blendShaderID)->Bind(cmdBuffer);
                material->Bind(cmdBuffer);
                vkCmdDispatch(cmdBuffer, (width + 15) / 16,
                (height + 15) / 16, 1);
            });
        }
        );

    renderPassManager.FindRenderPass(blendPass)->SetInputOutputResources(colorAttachment, blendAttachment).SetReadResource(generateColAttachments[0]);
    frameGraph->AddRenderPassNode(blendPass).AddResourceNode(blendAttachment);

    index1 = blendAttachment;
}
