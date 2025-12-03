//
// Created by 51092 on 25-8-23.
//

#include "LTCScene.h"
#include <vulkanFrameWork.h>

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

    api.CreateTexture(LTCTex1ID_, FrameWork::ResourceManager::GetInstance().LoadTextureFullData("../resources/Pic/LTCMap/ltc_1.dds", SFLOAT16));
    api.CreateTexture(LTCTex2ID_, FrameWork::ResourceManager::GetInstance().LoadTextureFullData("../resources/Pic/LTCMap/ltc_2.dds", SFLOAT16));

    frameGraph = std::make_unique<FG::FrameGraph>();
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

    //Shader Handle
    ltcLightShaderHandle = FrameWork::CaIShader::CreateHandle(ltcLightPath, VK_FORMAT_R16G16B16A16_SFLOAT);
    ltcFaceShaderHandle = FrameWork::CaIShader::CreateHandle(ltcFacePath, VK_FORMAT_R16G16B16A16_SFLOAT);
    presentShaderHandle = FrameWork::CaIShader::CreateHandle(presentPath);

    //Material Handle
    presentMaterialHandle = FrameWork::CaIMaterial::CreateHandle(presentShaderHandle);
    ltcFaceMaterialHandle = FrameWork::CaIMaterial::CreateHandle(ltcFaceShaderHandle);
    ltcLightMaterialHandle = FrameWork::CaIMaterial::CreateHandle(ltcLightShaderHandle);

    bloomPass = std::make_unique<FG::BloomingPass>(frameGraph.get(), 8, &threshold);


    api.GenFaceData(ltcLightModelID, {0, 0.5, 0}, {0, 0, 1}, 1, 1, "../resources/Pic/doro.png");
    api.GenFaceData(ltcFaceModelID, {0, 0.0, 0}, {0, 1, 0}, 20, 20);


    colorAttachmentID = frameGraph->GetResourceManager().RegisterResource(
        [&](std::unique_ptr<FG::ResourceDescription>& resource) {
            resource->SetName("colorAttachment");
            resource->SetDescription<FG::TextureDescription>(
                std::make_unique<FG::TextureDescription>(
                    api.windowWidth, api.windowHeight, VK_FORMAT_R16G16B16A16_SFLOAT, 1, 1, VK_SAMPLE_COUNT_1_BIT,
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT
                    )
                );
        }
        );

    depthAttachmentID = frameGraph->GetResourceManager().RegisterResource(
    [&](std::unique_ptr<FG::ResourceDescription>& desc) {
        desc->SetName("DepthAttachment")
        .SetDescription<FG::TextureDescription>(
            std::make_unique<FG::TextureDescription>(
                vulkanRenderAPI.GetFrameWidth(), vulkanRenderAPI.GetFrameHeight(),
                vulkanRenderAPI.GetDepthFormat() , 1, 1, VK_SAMPLE_COUNT_1_BIT,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
                )
            );
    }
    );

    colorAttachmentID1 = frameGraph->GetResourceManager().RegisterResource(
    [&](std::unique_ptr<FG::ResourceDescription>& resource) {
        resource->SetName("colorAttachment1");
        resource->SetDescription<FG::TextureDescription>(
            std::make_unique<FG::TextureDescription>(
                api.windowWidth, api.windowHeight, VK_FORMAT_R16G16B16A16_SFLOAT, 1, 1, VK_SAMPLE_COUNT_1_BIT,
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT
                )
            );
    }
    );

    depthAttachmentID1 = frameGraph->GetResourceManager().RegisterResource(
    [&](std::unique_ptr<FG::ResourceDescription>& desc) {
        desc->SetName("DepthAttachment1")
        .SetDescription<FG::TextureDescription>(
            std::make_unique<FG::TextureDescription>(
                vulkanRenderAPI.GetFrameWidth(), vulkanRenderAPI.GetFrameHeight(),
                vulkanRenderAPI.GetDepthFormat() , 1, 1, VK_SAMPLE_COUNT_1_BIT,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
                )
            );
    }
    );

    swapChainAttachmentID = frameGraph->GetResourceManager().RegisterResource([&](std::unique_ptr<FG::ResourceDescription> &desc) {
        desc->SetName("swapChain");
        desc->SetDescription<FG::TextureDescription>(std::make_unique<FG::TextureDescription>());
        desc->isExternal = true;
        desc->isPresent = true;
        desc->vulkanIndex = vulkanRenderAPI.GetSwapChainTextures()[0];
    });

    auto BindCamera = [](FrameWork::Handle<FrameWork::CaIMaterial> materialHandle, FrameWork::Camera* camera,uint32_t modelID) {
        auto model = vulkanRenderAPI.getByIndex<FrameWork::VulkanModelData>(modelID);
        glm::mat4 projection = glm::perspective(glm::radians(camera->Zoom),
                              (float) vulkanRenderAPI.windowWidth / (float) vulkanRenderAPI.windowHeight,
                              0.01f, 100.0f);
        projection[1][1] *= -1;
        glm::mat4 pos = glm::translate(glm::mat4(1.0), model->position);
        FrameWork::CaIMaterial::SetParam(materialHandle, "viewMatrix", camera->GetViewMatrix(), 0);
        FrameWork::CaIMaterial::SetParam(materialHandle, "projectionMatrix", projection, 0);
        FrameWork::CaIMaterial::SetParam(materialHandle, "modelMatrix", pos, 0);
    };

    auto UpdateLightUniform = [this, BindCamera](FrameWork::Handle<FrameWork::CaIMaterial> materialHandle, FrameWork::Camera* camera, uint32_t modelID) {
        BindCamera(materialHandle, camera, modelID);
        auto rotateY = glm::rotate(glm::mat4(1.0f), glm::radians(lightRotateY
        ), glm::vec3(0.0f, 1.0f, 0.0f));
        auto rotateX = glm::rotate(glm::mat4(1.0f), glm::radians(lightRotateX), glm::vec3(1.0f, 0.0f, 0.0f));
        auto scale = glm::scale(glm::mat4(1.0f), glm::vec3(lightScaleX, lightScaleY, 1.0f));
        auto pos = glm::translate(glm::mat4(1.0f), vulkanRenderAPI.getByIndex<FrameWork::VulkanModelData>(ltcLightModelID)->position);
        auto modelMatrix = pos * rotateX * rotateY * scale * glm::mat4(1.0f);
        FrameWork::CaIMaterial::SetParam(materialHandle, "lightMatrix", modelMatrix, 0);
        FrameWork::CaIMaterial::SetParam(materialHandle, "lightPosition", lightPos, 0);
        FrameWork::CaIMaterial::SetParam(materialHandle, "lightHeight", 1.0f, 0);
        FrameWork::CaIMaterial::SetParam(materialHandle, "lightWidth", 1.0f, 0);
        FrameWork::CaIMaterial::SetParam(materialHandle, "lightIntensity", intensity_, 0);
        FrameWork::CaIMaterial::SetParam(materialHandle, "cameraPos", camera->Position, 0);
        FrameWork::CaIMaterial::SetParam(materialHandle, "materialDiffuse", diffuse, 0);
        FrameWork::CaIMaterial::SetParam(materialHandle, "lightColor", lightColor, 0);
        FrameWork::CaIMaterial::SetParam(materialHandle, "materialF0", F0, 0);
        FrameWork::CaIMaterial::SetParam(materialHandle, "materialRoughness", roughness, 0);
        FrameWork::CaIMaterial::SetTexture(materialHandle, "LTC1", LTCTex1ID_);
        FrameWork::CaIMaterial::SetTexture(materialHandle, "LTC2", LTCTex2ID_);
        FrameWork::CaIMaterial::SetTexture(materialHandle, "lightTexture", vulkanRenderAPI.getByIndex<FrameWork::VulkanModelData>(ltcLightModelID)->textures.back()[DiffuseColor]);


    };

    ltcFacePass = frameGraph->GetRenderPassManager().RegisterRenderPass(
        [&](std::unique_ptr<FG::RenderPass>& renderPass) {
            renderPass->SetName("ltcFacePass");
            renderPass->SetExec(
                [this , UpdateLightUniform](VkCommandBuffer cmdBuffer) {
                    FrameWork::CaIShader::Bind(ltcFaceShaderHandle, cmdBuffer);
                    UpdateLightUniform(ltcFaceMaterialHandle, camera_, ltcFaceModelID);
                    FrameWork::CaIMaterial::Bind(ltcFaceMaterialHandle, cmdBuffer);
                    auto meshID = vulkanRenderAPI.
                    getByIndex<FrameWork::VulkanModelData>(ltcFaceModelID)->meshIDs[0];
                    vulkanRenderAPI.BindMesh(cmdBuffer, meshID);
                }
                );
        }
        );

    ltcLightPass = frameGraph->GetRenderPassManager().RegisterRenderPass(
        [&](std::unique_ptr<FG::RenderPass>& renderPass) {
            renderPass->SetName("ltcLightPass")
            .SetExec([this](VkCommandBuffer cmdBuffer) {
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
                FrameWork::CaIMaterial::SetParam(ltcLightMaterialHandle, "viewMatrix", camera_->GetViewMatrix(), 0);
                FrameWork::CaIMaterial::SetParam(ltcLightMaterialHandle, "projectionMatrix", projection, 0);
                FrameWork::CaIMaterial::SetParam(ltcLightMaterialHandle, "modelMatrix", modelMatrix, 0);
                FrameWork::CaIMaterial::SetParam(ltcLightMaterialHandle, "intensity", intensity_, 0);
                FrameWork::CaIMaterial::SetTexture(ltcLightMaterialHandle, "colorSampler", vulkanRenderAPI.getByIndex<FrameWork::VulkanModelData>(ltcLightModelID)->textures.back()[DiffuseColor]);
                FrameWork::CaIShader::Bind(ltcLightShaderHandle, cmdBuffer);
                FrameWork::CaIMaterial::Bind(ltcLightMaterialHandle, cmdBuffer);
                auto meshID = vulkanRenderAPI.
                getByIndex<FrameWork::VulkanModelData>(ltcLightModelID)->meshIDs[0];
                vulkanRenderAPI.BindMesh(cmdBuffer, meshID);
            });
        }
        );
    bloomPass->SetInputOutputResource(colorAttachmentID1, bloomingAttachment);

    presentPass = frameGraph->GetRenderPassManager().RegisterRenderPass([this](auto &renderPass) {
        renderPass->SetName("presentPass");
        renderPass->SetExec([&](VkCommandBuffer cmdBuffer) {
        //绑定对应imageView
        FrameWork::CaIShader::Bind(presentShaderHandle, cmdBuffer);
        FrameWork::CaIMaterial::SetAttachment(presentMaterialHandle, "colorTexture", frameGraph->GetResourceManager().GetVulkanIndex(bloomingAttachment));
        FrameWork::CaIMaterial::Bind(presentMaterialHandle, cmdBuffer);
        vkCmdDraw(cmdBuffer, 6, 1, 0, 0);
            });
        });

    //连接FrameGraph
    frameGraph->AddResourceNode(colorAttachmentID)
    .AddResourceNode(swapChainAttachmentID).AddResourceNode(depthAttachmentID)
    .AddResourceNode(colorAttachmentID1).AddResourceNode(depthAttachmentID1)
    .AddRenderPassNode(ltcFacePass).AddRenderPassNode(ltcLightPass).AddRenderPassNode(presentPass);

    frameGraph->SetUpdateBeforeRendering([this]() {
    auto swapChainDesc = frameGraph->GetResourceManager().FindResource(swapChainAttachmentID);
    swapChainDesc->vulkanIndex = vulkanRenderAPI.GetSwapChainTextures()[vulkanRenderAPI.GetCurrentImageIndex()];
    //防止窗口更新不对齐
    swapChainDesc->GetDescription<FG::TextureDescription>()->width = vulkanRenderAPI.windowWidth;
    swapChainDesc->GetDescription<FG::TextureDescription>()->height = vulkanRenderAPI.windowHeight;
    });

    frameGraph->GetRenderPassManager().FindRenderPass(ltcFacePass)->SetCreateResource(colorAttachmentID).SetCreateResource(depthAttachmentID);
    frameGraph->GetRenderPassManager().FindRenderPass(ltcLightPass)->SetInputOutputResources(colorAttachmentID, colorAttachmentID1)
    .SetInputOutputResources(depthAttachmentID,depthAttachmentID1);
    frameGraph->GetRenderPassManager().FindRenderPass(presentPass)->SetCreateResource(swapChainAttachmentID).SetReadResource(
    bloomingAttachment);

    frameGraph->Compile();
}
