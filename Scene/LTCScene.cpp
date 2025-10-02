//
// Created by 51092 on 25-8-23.
//

#include "LTCScene.h"
#include <vulkanFrameWork.h>
#include <array>

LTCScene::LTCScene(FrameWork::Camera *camera) {
    auto& api = vulkanRenderAPI;
    camera_ = camera;
    GUIFunc = [this] {
        ImGui::SliderFloat("Intensity", &intensity_, 0.0f, 10.0f);
        ImGui::SliderFloat("ScaleX", &lightScaleX, 0.0f, 10.0f);
        ImGui::SliderFloat("ScaleY", &lightScaleY, 0.0f, 10.0f);
        ImGui::SliderFloat("RotateX", &lightRotateX, 0.0f, 360.0f);
        ImGui::SliderFloat("RotateY", &lightRotateY, 0.0f, 360.0f);
        ImGui::SliderFloat("Roughness", &roughness, 0.01f, 1.0f);
        ImGui::ColorEdit3("Diffuse", &diffuse[0]);
        ImGui::SliderFloat("Blooming Thres", &threshold, 0.01f, 2.0f);
    };

    api.CreateTexture(LTCTex1ID_, FrameWork::Resource::GetInstance().LoadTextureFullData("../resources/Pic/LTCMap/ltc_1.dds", SFLOAT16));
    api.CreateTexture(LTCTex2ID_, FrameWork::Resource::GetInstance().LoadTextureFullData("../resources/Pic/LTCMap/ltc_2.dds", SFLOAT16));

    frameGraph = std::make_unique<FG::FrameGraph>(resourceManager, renderPassManager);
    CreateFrameGraphResource();

}

LTCScene::~LTCScene() {
}

void LTCScene::Render(const VkCommandBuffer &cmdBuffer) {


    frameGraph->Execute(cmdBuffer);
}

const std::function<void()> & LTCScene::GetRenderFunction() {
    return GUIFunc;
}

std::string LTCScene::GetName() const {
    return "LTCScene";
}



void LTCScene::CreateFrameGraphResource() {
    auto& api = vulkanRenderAPI;
    std::string ltcFacePath = "../resources/CaIShaders/LTC/LTCFace.caishader";
    std::string ltcLightPath = "../resources/CaIShaders/LTC/LTCLight.caishader";
    std::string presentPath = "../resources/CaIShaders/Present/present.caishader";

    FrameWork::CaIShader::Create(ltcLightShaderID, ltcLightPath, VK_FORMAT_R32G32B32A32_SFLOAT);
    FrameWork::CaIShader::Create(ltcFaceShaderID, ltcFacePath, VK_FORMAT_R32G32B32A32_SFLOAT);
    FrameWork::CaIShader::Create(presentShaderID, presentPath);
    FrameWork::CaIMaterial::Create(presentMaterialID, presentShaderID);
    FrameWork::CaIMaterial::Create(ltcFaceMaterialID, ltcFaceShaderID);
    FrameWork::CaIMaterial::Create(ltcLightMaterialID, ltcLightShaderID);
    bloomPass = std::make_unique<FG::BloomingPass>(frameGraph.get(), 8, &threshold);


    api.GenFaceData(ltcLightModelID, {0, 0.5, 0}, {0, 0, 1}, 1, 1, "../resources/Pic/doro.png");
    api.GenFaceData(ltcFaceModelID, {0, 0.0, 0}, {0, 1, 0}, 20, 20);


    colorAttachmentID = resourceManager.RegisterResource(
        [&](std::unique_ptr<FG::ResourceDescription>& resource) {
            resource->SetName("colorAttachment");
            resource->SetDescription<FG::TextureDescription>(
                std::make_unique<FG::TextureDescription>(
                    api.windowWidth, api.windowHeight, VK_FORMAT_R32G32B32A32_SFLOAT, 1, 1 ,1, VK_SAMPLE_COUNT_1_BIT,
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT
                    )
                );
        }
        );

    depthAttachmentID = resourceManager.RegisterResource(
    [&](std::unique_ptr<FG::ResourceDescription>& desc) {
        desc->SetName("DepthAttachment")
        .SetDescription<FG::TextureDescription>(
            std::make_unique<FG::TextureDescription>(
                vulkanRenderAPI.GetFrameWidth(), vulkanRenderAPI.GetFrameHeight(),
                vulkanRenderAPI.GetDepthFormat() , 1,1, 1, VK_SAMPLE_COUNT_1_BIT,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
                )
            );
    }
    );

    colorAttachmentID1 = resourceManager.RegisterResource(
    [&](std::unique_ptr<FG::ResourceDescription>& resource) {
        resource->SetName("colorAttachment1");
        resource->SetDescription<FG::TextureDescription>(
            std::make_unique<FG::TextureDescription>(
                api.windowWidth, api.windowHeight, VK_FORMAT_R32G32B32A32_SFLOAT, 1,1, 1, VK_SAMPLE_COUNT_1_BIT,
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT
                )
            );
    }
    );

    depthAttachmentID1 = resourceManager.RegisterResource(
    [&](std::unique_ptr<FG::ResourceDescription>& desc) {
        desc->SetName("DepthAttachment1")
        .SetDescription<FG::TextureDescription>(
            std::make_unique<FG::TextureDescription>(
                vulkanRenderAPI.GetFrameWidth(), vulkanRenderAPI.GetFrameHeight(),
                vulkanRenderAPI.GetDepthFormat() , 1,1, 1, VK_SAMPLE_COUNT_1_BIT,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
                )
            );
    }
    );

    swapChainAttachmentID = resourceManager.RegisterResource([&](std::unique_ptr<FG::ResourceDescription> &desc) {
        desc->SetName("swapChain");
        desc->SetDescription<FG::TextureDescription>(std::make_unique<FG::TextureDescription>());
        desc->isExternal = true;
        desc->isPresent = true;
        desc->vulkanIndex = vulkanRenderAPI.GetSwapChainTextures()[0];
    });

    auto BindCamera = [](uint32_t materialID, FrameWork::Camera* camera,uint32_t modelID) {
        auto model = vulkanRenderAPI.getByIndex<FrameWork::VulkanModelData>(modelID);
        glm::mat4 projection = glm::perspective(glm::radians(camera->Zoom),
                              (float) vulkanRenderAPI.windowWidth / (float) vulkanRenderAPI.windowHeight,
                              0.01f, 100.0f);
        projection[1][1] *= -1;
        glm::mat4 pos = glm::translate(glm::mat4(1.0), model->position);
        auto material = FrameWork::CaIMaterial::Get(materialID);
        material->SetParam("viewMatrix", camera->GetViewMatrix(), 0);
        material->SetParam("projectionMatrix", projection, 0);
        material->SetParam("modelMatrix", pos, 0);
    };

    auto UpdateLightUniform = [this, BindCamera](uint32_t materialID, FrameWork::Camera* camera, uint32_t modelID) {
        BindCamera(materialID, camera, modelID);
        auto rotateY = glm::rotate(glm::mat4(1.0f), glm::radians(lightRotateY
        ), glm::vec3(0.0f, 1.0f, 0.0f));
        auto rotateX = glm::rotate(glm::mat4(1.0f), glm::radians(lightRotateX), glm::vec3(1.0f, 0.0f, 0.0f));
        auto scale = glm::scale(glm::mat4(1.0f), glm::vec3(lightScaleX, lightScaleY, 1.0f));
        auto pos = glm::translate(glm::mat4(1.0f), vulkanRenderAPI.getByIndex<FrameWork::VulkanModelData>(ltcLightModelID)->position);
        auto modelMatrix = pos * rotateX * rotateY * scale * glm::mat4(1.0f);
        auto material = FrameWork::CaIMaterial::Get(materialID);
        material->SetParam("lightMatrix", modelMatrix, 0)
        .SetParam("lightPosition", lightPos, 0)
        .SetParam("lightHeight", 1.0f, 0)
        .SetParam("lightWidth", 1.0f, 0)
        .SetParam("lightIntensity", intensity_, 0)
        .SetParam("cameraPos", camera_->Position, 0)
        .SetParam("materialDiffuse", diffuse, 0)
        .SetParam("lightColor", lightColor, 0)
        .SetParam("materialF0", F0, 0)
        .SetParam("materialRoughness", roughness, 0)
        .SetTexture("LTC1", LTCTex1ID_)
        .SetTexture("LTC2", LTCTex2ID_)
        .SetTexture("lightTexture", vulkanRenderAPI.getByIndex<FrameWork::VulkanModelData>(ltcLightModelID)->textures.back()[DiffuseColor]);


    };

    ltcFacePass = renderPassManager.RegisterRenderPass(
        [&](std::unique_ptr<FG::RenderPass>& renderPass) {
            renderPass->SetName("ltcFacePass");
            renderPass->SetExec(
                [this , UpdateLightUniform](VkCommandBuffer cmdBuffer) {
                    FrameWork::CaIShader::Get(ltcFaceShaderID)->Bind(cmdBuffer);
                    auto material = FrameWork::CaIMaterial::Get(ltcFaceMaterialID);
                    UpdateLightUniform(ltcFaceMaterialID, camera_, ltcFaceModelID);
                    material->Bind(cmdBuffer);
                    auto meshID = vulkanRenderAPI.
                    getByIndex<FrameWork::VulkanModelData>(ltcFaceModelID)->meshIDs[0];
                    vulkanRenderAPI.BindMesh(cmdBuffer, meshID);
                }
                );
        }
        );

    ltcLightPass = renderPassManager.RegisterRenderPass(
        [&](std::unique_ptr<FG::RenderPass>& renderPass) {
            renderPass->SetName("ltcLightPass")
            .SetExec([this](VkCommandBuffer cmdBuffer) {
                auto material = FrameWork::CaIMaterial::Get(ltcLightMaterialID);
                auto rotateY = glm::rotate(glm::mat4(1.0f), glm::radians(lightRotateY
                ), glm::vec3(0.0f, 1.0f, 0.0f));
                auto rotateX = glm::rotate(glm::mat4(1.0f), glm::radians(lightRotateX), glm::vec3(1.0f, 0.0f, 0.0f));
                auto scale = glm::scale(glm::mat4(1.0f), glm::vec3(lightScaleX, lightScaleY, 1.0f));
                auto pos = glm::translate(glm::mat4(1.0f), vulkanRenderAPI.getByIndex<FrameWork::VulkanModelData>(ltcLightModelID)->position);
                auto modelMatrix = pos * rotateX * rotateY * scale * glm::mat4(1.0f);
                glm::mat4 projection = glm::perspective(glm::radians(camera_->Zoom),
                      (float) vulkanRenderAPI.windowWidth / (float) vulkanRenderAPI.windowHeight,
                      0.01f, 100.0f);
                projection[1][1] *= -1;
                material->SetParam("viewMatrix", camera_->GetViewMatrix(), 0);
                material->SetParam("projectionMatrix", projection, 0);
                material->SetParam("modelMatrix", modelMatrix, 0);
                material->SetParam("intensity", intensity_, 0)
                .SetTexture("colorSampler", vulkanRenderAPI.getByIndex<FrameWork::VulkanModelData>(ltcLightModelID)->textures.back()[DiffuseColor]);
                FrameWork::CaIShader::Get(ltcLightShaderID)->Bind(cmdBuffer);
                material->Bind(cmdBuffer);
                auto meshID = vulkanRenderAPI.
                getByIndex<FrameWork::VulkanModelData>(ltcLightModelID)->meshIDs[0];
                vulkanRenderAPI.BindMesh(cmdBuffer, meshID);
            });
        }
        );
    bloomPass->SetInputOutputResource(colorAttachmentID1, bloomingAttachment);

    presentPass = renderPassManager.RegisterRenderPass([this](auto &renderPass) {
        renderPass->SetName("presentPass");
        renderPass->SetExec([&](VkCommandBuffer cmdBuffer) {
        //绑定对应imageView
        FrameWork::CaIShader::Get(presentShaderID)->Bind(cmdBuffer);
        FrameWork::CaIMaterial::Get(presentMaterialID)->SetAttachment("colorTexture", resourceManager.GetVulkanIndex(bloomingAttachment));
        FrameWork::CaIMaterial::Get(presentMaterialID)->Bind(cmdBuffer);
        vkCmdDraw(cmdBuffer, 6, 1, 0, 0);
            });
        });

    //连接FrameGraph
    frameGraph->AddResourceNode(colorAttachmentID)
    .AddResourceNode(swapChainAttachmentID).AddResourceNode(depthAttachmentID)
    .AddResourceNode(colorAttachmentID1).AddResourceNode(depthAttachmentID1)
    .AddRenderPassNode(ltcFacePass).AddRenderPassNode(ltcLightPass).AddRenderPassNode(presentPass);

    frameGraph->SetUpdateBeforeRendering([this]() {
    auto swapChainDesc = resourceManager.FindResource(swapChainAttachmentID);
    swapChainDesc->vulkanIndex = vulkanRenderAPI.GetSwapChainTextures()[vulkanRenderAPI.GetCurrentImageIndex()];
    //防止窗口更新不对齐
    swapChainDesc->GetDescription<FG::TextureDescription>()->width = vulkanRenderAPI.windowWidth;
    swapChainDesc->GetDescription<FG::TextureDescription>()->height = vulkanRenderAPI.windowHeight;
    });

    bloomPass->Bind();
    renderPassManager.FindRenderPass(ltcFacePass)->SetCreateResource(colorAttachmentID).SetCreateResource(depthAttachmentID);
    renderPassManager.FindRenderPass(ltcLightPass)->SetInputOutputResources(colorAttachmentID, colorAttachmentID1)
    .SetInputOutputResources(depthAttachmentID,depthAttachmentID1);
    renderPassManager.FindRenderPass(presentPass)->SetCreateResource(swapChainAttachmentID).SetReadResource(
    bloomingAttachment);

    frameGraph->Compile();
}
