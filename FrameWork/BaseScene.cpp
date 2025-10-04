//
// Created by 51092 on 25-8-19.
//

#include "BaseScene.h"
#include "vulkanFrameWork.h"
#include <iostream>

#include "CompShader.h"
#include "FrameGraph/FrameGraph.h"
#include "CompMaterial.h"

BaseScene::BaseScene(FrameWork::Camera& camera) {
    cameraPtr = &camera;
    GUIFunc = [this]() {

    };
    sceneName =  "Base Scene";
    //初始化frameGraph
    frameGraph = std::make_unique<FG::FrameGraph>();
    CreateFrameGraphResource();

}

BaseScene::~BaseScene() {
    aabbDeBugging.Destroy();
}




void BaseScene::Render(const VkCommandBuffer& cmdBuffer) {

    frameGraph->Execute(cmdBuffer);
}

const std::function<void()> & BaseScene::GetRenderFunction() {
    return GUIFunc;
}

std::string BaseScene::GetName() const {
    return sceneName;
}


void BaseScene::CreateFrameGraphResource() {
    //创建持久资源
    std::string shaderPath = "../resources/CaIShaders/BaseScene/BaseScene.caishader";
    std::string testShaderPath = "../resources/CaIShaders/TestFrameGraph/forward.caishader";
    std::string testCompShaderPath = "../resources/CaIShaders/Bloom/downSample.compshader";

    FrameWork::CaIShader::Create(caiShaderID, shaderPath, VK_FORMAT_R16G16B16A16_SFLOAT);
    FrameWork::CompShader::Create(compShaderID, testCompShaderPath);
    compMaterials.resize(7);
    for (auto& compMaterial : compMaterials) {
        FrameWork::CompMaterial::Create(compMaterial, compShaderID);
    }
    vulkanRenderAPI.LoadVulkanModel(vulkanModelID, "cocona", ModelType::OBJ, DiffuseColor,
        {0,0, 0}, 1.0f);
    auto model = vulkanRenderAPI.getByIndex<FrameWork::VulkanModelData>(vulkanModelID);
    // 绑定静态纹理
     materials.resize(model->meshIDs.size());
     for (int i = 0; i < model->meshIDs.size(); i++) {
         FrameWork::CaIMaterial::Create(materials[i], caiShaderID);
         FrameWork::CaIMaterial::Get(materials[i])->SetTexture("colorSampler", model->textures[i][DiffuseColor]);
     }
    std::string presentShaderPath = "../resources/CaIShaders/Present/Present.caishader";
    FrameWork::CaIShader::Create(presentShaderID, presentShaderPath, vulkanRenderAPI.GetVulkanSwapChain().colorFormat);
    FrameWork::CaIShader::Create(resolveShaderID, presentShaderPath, VK_FORMAT_R16G16B16A16_SFLOAT);
    FrameWork::CaIMaterial::Create(presentMaterialID, presentShaderID);
    FrameWork::CaIMaterial::Create(resolveMaterialID, resolveShaderID);

    //创建FrameGraph资源
    colorAttachment = frameGraph->GetResourceManager().RegisterResource(
        [&](std::unique_ptr<FG::ResourceDescription>& desc) {
            desc->SetName("ColorAttachment")
            .SetDescription<FG::TextureDescription>(
                std::make_unique<FG::TextureDescription>(
                    vulkanRenderAPI.GetFrameWidth(), vulkanRenderAPI.GetFrameHeight(),
                    VK_FORMAT_R16G16B16A16_SFLOAT, 1, 1, vulkanRenderAPI.GetSampleCount(),
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
                    )
                );
        }
        );

    resolveAttachment = frameGraph->GetResourceManager().RegisterResource(
        [&](std::unique_ptr<FG::ResourceDescription>& desc) {
            desc->SetName("resolveAttachment")
            .SetDescription<FG::TextureDescription>(
                std::make_unique<FG::TextureDescription>(
                    vulkanRenderAPI.GetFrameWidth(), vulkanRenderAPI.GetFrameHeight(),
                    VK_FORMAT_R16G16B16A16_SFLOAT,  1, 1, VK_SAMPLE_COUNT_1_BIT,
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
                    )
                );
        }
        );

    depthAttachment = frameGraph->GetResourceManager().RegisterResource(
        [&](std::unique_ptr<FG::ResourceDescription>& desc) {
            desc->SetName("DepthAttachment")
            .SetDescription<FG::TextureDescription>(
                std::make_unique<FG::TextureDescription>(
                    vulkanRenderAPI.GetFrameWidth(), vulkanRenderAPI.GetFrameHeight(),
                    vulkanRenderAPI.GetDepthFormat() , 1, 1, vulkanRenderAPI.GetSampleCount(),
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
                    )
                );
        }
        );

    swapChainAttachment = frameGraph->GetResourceManager().RegisterResource([&](std::unique_ptr<FG::ResourceDescription> &desc) {
        desc->SetName("swapChain");
        desc->SetDescription<FG::TextureDescription>(std::make_unique<FG::TextureDescription>());
        desc->isExternal = true;
        desc->isPresent = true;
        desc->vulkanIndex = vulkanRenderAPI.GetSwapChainTextures()[0]; //先随便绑定一个
    });

    auto forwardPass = frameGraph->GetRenderPassManager().RegisterRenderPass([this](std::unique_ptr<FG::RenderPass> &renderPass) {
    renderPass->SetName("forwardPass");
    renderPass->SetExec([&](VkCommandBuffer cmdBuffer) {
            FrameWork::CaIShader::Get(caiShaderID)->Bind(cmdBuffer);
            auto model = vulkanRenderAPI.getByIndex<FrameWork::VulkanModelData>(vulkanModelID);
            glm::mat4 projection = glm::perspective(glm::radians(cameraPtr->Zoom),
                                  (float) vulkanRenderAPI.windowWidth / (float) vulkanRenderAPI.windowHeight,
                                  0.01f, 100.0f);
            projection[1][1] *= -1;
            glm::mat4 pos = glm::translate(glm::mat4(1.0), model->position);
            for (int i = 0; i < model->meshIDs.size(); i++) {
                auto material = FrameWork::CaIMaterial::Get(materials[i]);
                material->SetParam("viewMatrix", cameraPtr->GetViewMatrix(), 0);
                material->SetParam("projectionMatrix", projection, 0);
                material->SetParam("modelMatrix", pos, 0);
                material->Bind(cmdBuffer);
                VkDeviceSize offsets[] = {0};
                auto mesh = vulkanRenderAPI.getByIndex<FrameWork::Mesh>(model->meshIDs[i]);
                vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &mesh->VertexBuffer.buffer, offsets);
                vkCmdBindIndexBuffer(cmdBuffer, mesh->IndexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(cmdBuffer, mesh->indexCount, 1, 0, 0, 0); //没有进行实例渲染
            }

        });
    });

    auto resolvePass = frameGraph->GetRenderPassManager().RegisterRenderPass([this](auto &renderPass) {
        renderPass->SetName("resolvePass").SetPassType(FG::PassType::Resolve);
        auto texDesc = frameGraph->GetResourceManager().FindResource(resolveAttachment)->GetDescription<FG::TextureDescription>();
        renderPass->SetExec([texDesc, this](VkCommandBuffer cmdBuffer) {

            //硬件Resolve
            VkImageResolve region{};
            region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
            region.srcOffset      = {0,0,0};
            region.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
            region.dstOffset      = {0,0,0};
            region.extent         = {texDesc->width, texDesc->height, 1};
            vkCmdResolveImage(
                cmdBuffer,
                vulkanRenderAPI.getByIndex<FrameWork::Texture>(frameGraph->GetResourceManager().GetVulkanIndex(colorAttachment))->image.image,   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                vulkanRenderAPI.getByIndex<FrameWork::Texture>(frameGraph->GetResourceManager().GetVulkanIndex(resolveAttachment))->image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &region);
                });
    });

    auto presentPass = frameGraph->GetRenderPassManager().RegisterRenderPass([this](auto &renderPass) {
        renderPass->SetName("presentPass");
        renderPass->SetExec([&](VkCommandBuffer cmdBuffer) {
            //绑定对应imageView
            FrameWork::CaIShader::Get(presentShaderID)->Bind(cmdBuffer);
            FrameWork::CaIMaterial::Get(presentMaterialID)->SetAttachment("colorTexture", frameGraph->GetResourceManager().GetVulkanIndex(resolveAttachment));
            FrameWork::CaIMaterial::Get(presentMaterialID)->Bind(cmdBuffer);
            vkCmdDraw(cmdBuffer, 6, 1, 0, 0);
        });
    });

    //构建图
    frameGraph->AddResourceNode(colorAttachment).AddResourceNode(swapChainAttachment).AddResourceNode(depthAttachment).AddResourceNode(resolveAttachment)
            .AddRenderPassNode(forwardPass).AddRenderPassNode(resolvePass).AddRenderPassNode(presentPass);



    //设置每帧资源更新
    frameGraph->SetUpdateBeforeRendering([this]() {
        auto swapChainDesc = frameGraph->GetResourceManager().FindResource(swapChainAttachment);
        swapChainDesc->vulkanIndex = vulkanRenderAPI.GetSwapChainTextures()[vulkanRenderAPI.GetCurrentImageIndex()];
        //防止窗口更新不对齐
        swapChainDesc->GetDescription<FG::TextureDescription>()->width = vulkanRenderAPI.windowWidth;
        swapChainDesc->GetDescription<FG::TextureDescription>()->height = vulkanRenderAPI.windowHeight;
    });

    frameGraph->GetRenderPassManager().FindRenderPass(forwardPass)->SetCreateResource(colorAttachment).SetCreateResource(depthAttachment);
    frameGraph->GetRenderPassManager().FindRenderPass(resolvePass)->SetCreateResource(resolveAttachment).SetReadResource(colorAttachment);

    frameGraph->GetRenderPassManager().FindRenderPass(presentPass)->SetCreateResource(swapChainAttachment).SetReadResource(resolveAttachment);

    frameGraph->Compile();
}
