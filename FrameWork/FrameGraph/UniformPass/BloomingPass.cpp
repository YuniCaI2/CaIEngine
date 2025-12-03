//
// Created by 51092 on 2025/9/30.
//

#include "BloomingPass.h"
#include "../../CompShader.h"
#include "../../CompMaterial.h"

FG::BloomingPass::~BloomingPass() {
}

FG::BloomingPass::BloomingPass(FrameGraph *frameGraph, uint32_t mipmapLevels, float*  thres): downSampling(frameGraph, mipmapLevels) {
    this->mipmapLevels = mipmapLevels;
    this->frameGraph = frameGraph;
    this->threshold = thres;


    //MaterialHandle
    compColMaterialHandles.resize(mipmapLevels);
    compRowMaterialHandles.resize(mipmapLevels);


    //ShaderHandle
    rowShaderHandle = FrameWork::CompShader::CreateHandle(rowShaderPath.data());
    colShaderHandle = FrameWork::CompShader::CreateHandle(colShaderPath.data());
    specShaderHandle = FrameWork::CompShader::CreateHandle(specShaderPath.data());
    blendShaderHandle = FrameWork::CompShader::CreateHandle(blendShaderPath.data());
    for (int i = 0; i < mipmapLevels; i++) {
        //MaterialHandle
        compColMaterialHandles[i] = FrameWork::CompMaterial::CreateHandle(colShaderHandle);
        compRowMaterialHandles[i] = FrameWork::CompMaterial::CreateHandle(rowShaderHandle);
    }
    //MaterialHandle
    getSpecMaterialHandle = FrameWork::CompMaterial::CreateHandle(specShaderHandle);
    blendMaterialHandle = FrameWork::CompMaterial::CreateHandle(blendShaderHandle);

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
                texDesc->format, mipmapLevels, 1, texDesc->samples, //一般接受的是一个resolve,采样保证为1
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
                texDesc->format, mipmapLevels, 1, texDesc->samples, //一般接受的是一个resolve,采样保证为1
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
                texDesc->format, mipmapLevels ,1, texDesc->samples, //一般接受的是一个resolve,采样保证为1
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
                    texDesc->format, mipmapLevels, 1, texDesc->samples, //一般接受的是一个resolve,采样保证为1
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
                texDesc->format, texDesc->mipLevels, texDesc->arrayLayers, texDesc->samples, //和colorAttachment一致,作为colorAttachment在FrameGraph的替身
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
                    float thres = threshold ? *threshold : 1.0f;
                    uint32_t width = texDesc->width;
                    uint32_t height = texDesc->height;
                    //Handle
                    FrameWork::CompShader::Bind(specShaderHandle, cmdBuffer);
                    //MaterialHandle
                    FrameWork::CompMaterial::SetAttachment(getSpecMaterialHandle,
                        "srcImage", resourceManager.GetVulkanIndex(colorAttachment)
                        );
                    FrameWork::CompMaterial::SetStorageImage2D(getSpecMaterialHandle,
                        "dstImage", resourceManager.GetVulkanIndex(specAttachment)
                        );

                    FrameWork::CompMaterial::SetParam(getSpecMaterialHandle, "threshold", thres);
                    FrameWork::CompMaterial::SetParam(getSpecMaterialHandle, "dstScale", glm::vec2(width, height));
                    FrameWork::CompMaterial::SetParam(getSpecMaterialHandle, "invDstScale", glm::vec2(1.0f / width, 1.0f / height));
                    FrameWork::CompMaterial::Bind(getSpecMaterialHandle, cmdBuffer);


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
                    //ShaderHandle
                    FrameWork::CompShader::Bind(rowShaderHandle, cmdBuffer);

                    //MaterialHandle
                    FrameWork::CompMaterial::SetStorageImage2D(compRowMaterialHandles[i],"dstImage", resourceManager.GetVulkanIndex(generateRowAttachment), i);
                    FrameWork::CompMaterial::SetTexture(compRowMaterialHandles[i],"srcImage", resourceManager.GetVulkanIndex(downSamplingAttachment));
                    FrameWork::CompMaterial::SetParam(compRowMaterialHandles[i],"dstLod", static_cast<float>(i));
                    FrameWork::CompMaterial::SetParam(compRowMaterialHandles[i],"dstScale", glm::vec2(width, height));
                    FrameWork::CompMaterial::SetParam(compRowMaterialHandles[i],"invDstScale", glm::vec2(1.0f / width, 1.0f / height));
                    FrameWork::CompMaterial::Bind(compRowMaterialHandles[i], cmdBuffer);
                    
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
                    //Handle
                    FrameWork::CompShader::Bind(colShaderHandle, cmdBuffer);
                    //MaterialHandle
                    FrameWork::CompMaterial::SetStorageImage2D(compColMaterialHandles[i],"dstImage", resourceManager.GetVulkanIndex(generateColAttachments[i]), i);
                    FrameWork::CompMaterial::SetTexture(compColMaterialHandles[i],"srcImage", resourceManager.GetVulkanIndex(generateRowAttachment));
                    FrameWork::CompMaterial::SetTexture(compColMaterialHandles[i],"blendImage", resourceManager.GetVulkanIndex(generateColAttachments[i]));
                    FrameWork::CompMaterial::SetParam(compColMaterialHandles[i],"maxLod", static_cast<float>(mipmapLevels - 1));
                    FrameWork::CompMaterial::SetParam(compColMaterialHandles[i],"dstLod", static_cast<float>(i));
                    FrameWork::CompMaterial::SetParam(compColMaterialHandles[i],"dstScale", glm::vec2(width, height));
                    FrameWork::CompMaterial::SetParam(compColMaterialHandles[i],"invDstScale", glm::vec2(1.0f / width, 1.0f / height));
                    FrameWork::CompMaterial::Bind(compColMaterialHandles[i], cmdBuffer);

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

    //Blend
    blendPass = renderPassManager.RegisterRenderPass(
        [&](std::unique_ptr<RenderPass>& renderPass) {
            renderPass->SetName("blendPass")
            .SetPassType(PassType::Compute)
            .SetExec([&, texDesc](VkCommandBuffer cmdBuffer) {
                uint32_t width = texDesc->width;
                uint32_t height = texDesc->height;
                //Handle
                FrameWork::CompShader::Bind(blendShaderHandle, cmdBuffer);
                //MaterialHandle
                FrameWork::CompMaterial::SetTexture(blendMaterialHandle, "srcImage", resourceManager.GetVulkanIndex(generateColAttachments[0]));
                FrameWork::CompMaterial::SetStorageImage2D(blendMaterialHandle, "dstImage", resourceManager.GetVulkanIndex(colorAttachment), 0);
                FrameWork::CompMaterial::SetParam(blendMaterialHandle, "dstScale", glm::vec2(width, height));
                FrameWork::CompMaterial::SetParam(blendMaterialHandle, "invDstScale", glm::vec2(1.0f / width, 1.0f / height));
                FrameWork::CompMaterial::Bind(blendMaterialHandle, cmdBuffer);

                vkCmdDispatch(cmdBuffer, (width + 15) / 16,
                (height + 15) / 16, 1);
            });
        }
        );

    renderPassManager.FindRenderPass(blendPass)->SetInputOutputResources(colorAttachment, blendAttachment).SetReadResource(generateColAttachments[0]);
    frameGraph->AddRenderPassNode(blendPass).AddResourceNode(blendAttachment);

    index1 = blendAttachment;
}
