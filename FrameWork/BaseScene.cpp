//
// Created by 51092 on 25-8-19.
//

#include "BaseScene.h"
#include "vulkanFrameWork.h"
#include <iostream>

#include "CompShader.h"
#include "FrameGraph/FrameGraph.h"


BaseScene::BaseScene(FrameWork::Camera& camera) {
    // CreateDescriptorSetLayout();
    // VkRenderPass renderPass = vulkanRenderAPI.GetRenderPass("forward");
    // CreateGraphicsPipeline();
    // PrepareResources(camera);
    cameraPtr = &camera;
    GUIFunc = [this]() {

    };
    sceneName =  "Base Scene";
    //初始化frameGraph
    frameGraph = std::make_unique<FG::FrameGraph>(resourceManager, renderPassManager);
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



void BaseScene::PrepareResources(FrameWork::Camera& camera) {

}

void BaseScene::CreateFrameGraphResource() {
    //创建持久资源
    std::string shaderPath = "../resources/CaIShaders/BaseScene/BaseScene.caishader";
    std::string testShaderPath = "../resources/CaIShaders/TestFrameGraph/forward.caishader";
    std::string testCompShaderPath = "../resources/CaIShaders/Bloom/downSample.compshader";

    FrameWork::CaIShader::Create(caiShaderID, shaderPath);
    FrameWork::CaIShader::Create(testShader, testShaderPath);
    FrameWork::CompShader::Create(compShaderID, testCompShaderPath);
    vulkanRenderAPI.LoadVulkanModel(vulkanModelID, "cocona", ModelType::OBJ, DiffuseColor, {0,0, 0}, 1.0f);
    auto model = vulkanRenderAPI.getByIndex<FrameWork::VulkanModelData>(vulkanModelID);
    // 绑定静态纹理
     materials.resize(model->meshIDs.size());
     for (int i = 0; i < model->meshIDs.size(); i++) {
         FrameWork::CaIMaterial::Create(materials[i], caiShaderID);
         FrameWork::CaIMaterial::Get(materials[i])->SetTexture("colorSampler", model->textures[i][DiffuseColor]);
     }
    std::string presentShaderPath = "../resources/CaIShaders/Present/Present.caishader";
    FrameWork::CaIShader::Create(presentShaderID, presentShaderPath, VK_FORMAT_R8G8B8A8_UNORM);
    FrameWork::CaIMaterial::Create(presentMaterialID, presentShaderID);

    //创建FrameGraph资源
    colorAttachment = resourceManager.RegisterResource(
        [&](std::unique_ptr<FG::ResourceDescription>& desc) {
            desc->SetName("ColorAttachment")
            .SetDescription<FG::TextureDescription>(
                std::make_unique<FG::TextureDescription>(
                    vulkanRenderAPI.GetFrameWidth(), vulkanRenderAPI.GetFrameHeight(),
                    VK_FORMAT_R8G8B8A8_UNORM, 1, 8, 1, vulkanRenderAPI.GetSampleCount(),
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
                    )
                );
        }
        );

    depthAttachment = resourceManager.RegisterResource(
        [&](std::unique_ptr<FG::ResourceDescription>& desc) {
            desc->SetName("DepthAttachment")
            .SetDescription<FG::TextureDescription>(
                std::make_unique<FG::TextureDescription>(
                    vulkanRenderAPI.GetFrameWidth(), vulkanRenderAPI.GetFrameHeight(),
                    vulkanRenderAPI.GetDepthFormat() , 1, 8, 1, vulkanRenderAPI.GetSampleCount(),
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
                    )
                );
        }
        );

    swapChainAttachment = resourceManager.RegisterResource([&](std::unique_ptr<FG::ResourceDescription> &desc) {
        desc->SetName("swapChain");
        desc->SetDescription<FG::TextureDescription>(std::make_unique<FG::TextureDescription>());
        desc->isExternal = true;
        desc->isPresent = true;
        desc->vulkanIndex = vulkanRenderAPI.GetSwapChainTextures()[0]; //先随便绑定一个
    });

    auto forwardPass = renderPassManager.RegisterRenderPass([this](std::unique_ptr<FG::RenderPass> &renderPass) {
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

    auto presentPass = renderPassManager.RegisterRenderPass([this](auto &renderPass) {
        renderPass->SetName("presentPass");
        renderPass->SetExec([&](VkCommandBuffer cmdBuffer) {
            //绑定对应imageView
            FrameWork::CaIShader::Get(presentShaderID)->Bind(cmdBuffer);
            FrameWork::CaIMaterial::Get(presentMaterialID)->SetAttachment("colorTexture", resourceManager.GetVulkanResolveIndex(colorAttachment));
            FrameWork::CaIMaterial::Get(presentMaterialID)->Bind(cmdBuffer);
            vkCmdDraw(cmdBuffer, 6, 1, 0, 0);
        });
    });

    //构建图
    frameGraph->AddResourceNode(colorAttachment).AddResourceNode(swapChainAttachment).AddResourceNode(depthAttachment)
            .AddRenderPassNode(forwardPass).AddRenderPassNode(presentPass);

    //设置每帧资源更新
    frameGraph->SetUpdateBeforeRendering([this]() {
        auto swapChainDesc = resourceManager.FindResource(swapChainAttachment);
        swapChainDesc->vulkanIndex = vulkanRenderAPI.GetSwapChainTextures()[vulkanRenderAPI.GetCurrentImageIndex()];
        //防止窗口更新不对齐
        swapChainDesc->GetDescription<FG::TextureDescription>()->width = vulkanRenderAPI.windowWidth;
        swapChainDesc->GetDescription<FG::TextureDescription>()->height = vulkanRenderAPI.windowHeight;

        auto colorAttachDesc = resourceManager.FindResource(colorAttachment);
        auto depthAttachDesc = resourceManager.FindResource(depthAttachment);


    });

    renderPassManager.FindRenderPass(forwardPass)->SetCreateResource(colorAttachment).SetCreateResource(depthAttachment);
    renderPassManager.FindRenderPass(presentPass)->SetCreateResource(swapChainAttachment).SetReadResource(
        colorAttachment);

    frameGraph->Compile();
}
