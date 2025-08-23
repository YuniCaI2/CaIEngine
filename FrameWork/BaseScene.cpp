//
// Created by 51092 on 25-8-19.
//

#include "BaseScene.h"
#include "vulkanFrameWork.h"
#include "Slot.h"
#include <iostream>



BaseScene::BaseScene(FrameWork::Camera& camera) {
    CreateDescriptorSetLayout();
    VkRenderPass renderPass = vulkanRenderAPI.GetRenderPass("forward");
    CreateGraphicsPipeline();
    PrepareResources(camera);

}

BaseScene::~BaseScene() {
    aabbDeBugging.Destroy();
}




void BaseScene::Render(const VkCommandBuffer& cmdBuffer) {
    auto proj = glm::perspective(glm::radians(cameraPtr->Zoom),
                             (float) vulkanRenderAPI.windowWidth / (float) vulkanRenderAPI.windowHeight,
                             0.1f, 100.0f);
    proj[1][1] *= -1;
    aabbDeBugging.Update(cameraPtr->GetViewMatrix(), proj);
    if (useMSAA) {
        auto VulkanPipeline = vulkanRenderAPI.getByIndex<FrameWork::VulkanPipeline>(msaaPipelineID);
        graphicsPipeline = VulkanPipeline->pipeline;
        pipelineLayout = VulkanPipeline->pipelineLayout;
        vulkanRenderAPI.BeginRenderPass("forwardMSAA", msaaFrameBufferID,
            vulkanRenderAPI.GetFrameWidth(), vulkanRenderAPI.GetFrameHeight());

    }else {
        auto VulkanPipeline = vulkanRenderAPI.getByIndex<FrameWork::VulkanPipeline>(pipelineID);
        graphicsPipeline = VulkanPipeline->pipeline;
        pipelineLayout = VulkanPipeline->pipelineLayout;
        vulkanRenderAPI.BeginRenderPass("forward", frameBufferID,
            vulkanRenderAPI.GetFrameWidth(), vulkanRenderAPI.GetFrameHeight());
    }
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    auto globalSlot = vulkanRenderAPI.getByIndex<FrameWork::Slot>(globalSlotID);
    globalSlot->Bind(cmdBuffer, pipelineLayout, 0);

    for (int i = 0; i < modelID.size(); i++) {
        vulkanRenderAPI.DrawModel(modelID[i], cmdBuffer, pipelineLayout, globalSlot->GetDescriptorSetsSize());
    }
    vulkanRenderAPI.EndRenderPass();
    //---------------------------------------------------------------------------------
    if (displayAABB) {
        aabbDeBugging.Draw(cmdBuffer);
    }
}

const std::function<void()> & BaseScene::GetRenderFunction() {
    return GUIFunc;
}

std::vector<uint32_t> BaseScene::GetModelIDs() {
    return modelID;
}

std::string BaseScene::GetName() const {
    return sceneName;
}

uint32_t BaseScene::GetPresentColorAttachment() {
    return presentColorAttachment;
}

void BaseScene::CreateDescriptorSetLayout() {
    dynamicDescriptorSetLayout = FrameWork::Slot::CreateUniformDescriptorSetLayout(VK_SHADER_STAGE_VERTEX_BIT);
    textureDescriptorSetLayout = FrameWork::Slot::CreateTextureDescriptorSetLayout(VK_SHADER_STAGE_FRAGMENT_BIT);
}

void BaseScene::CreateGraphicsPipeline() {
    VkVertexInputBindingDescription bindingDescription = FrameWork::Vertex::getBindingDescription();
    auto attributeDescriptions = FrameWork::Vertex::getAttributeDescription();
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = vulkanRenderAPI.GetFrameWidth();
    viewport.height = vulkanRenderAPI.GetFrameHeight();
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = {vulkanRenderAPI.GetFrameWidth(), vulkanRenderAPI.GetFrameHeight()};

    uint32_t pipelineInfoId = -1;
    vulkanRenderAPI.InitPipelineInfo(pipelineInfoId);
    vulkanRenderAPI.LoadPipelineShader(pipelineInfoId, "triangle", VK_SHADER_STAGE_VERTEX_BIT);
    vulkanRenderAPI.LoadPipelineShader(pipelineInfoId, "triangle", VK_SHADER_STAGE_FRAGMENT_BIT);
    vulkanRenderAPI.AddPipelineVertexAttributeDescription(pipelineInfoId, attributeDescriptions);
    vulkanRenderAPI.AddPipelineVertexBindingDescription(pipelineInfoId, bindingDescription);
    vulkanRenderAPI.SetPipelineViewPort(pipelineInfoId, viewport);
    vulkanRenderAPI.SetPipelineScissor(pipelineInfoId, scissor);

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    vulkanRenderAPI.SetPipelineRasterizationState(pipelineInfoId, rasterizer);

    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    vulkanRenderAPI.SetPipelineDepthStencilState(pipelineInfoId, depthStencil);

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    vulkanRenderAPI.SetPipelineMultiSampleState(pipelineInfoId, multisampling);
    vulkanRenderAPI.AddPipelineColorBlendState(pipelineInfoId, true, BlendOp::Opaque);

    vulkanRenderAPI.CreateVulkanPipeline(pipelineID, "forwardPipeline", pipelineInfoId, "forward", 0,
                                         {dynamicDescriptorSetLayout,textureDescriptorSetLayout},
                                         2, 6);
    graphicsPipeline = vulkanRenderAPI.getByIndex<FrameWork::VulkanPipeline>(pipelineID)->pipeline;
    pipelineLayout = vulkanRenderAPI.getByIndex<FrameWork::VulkanPipeline>(pipelineID)->pipelineLayout;

    // 为MSAA创建完全独立的管线信息
    if (vulkanRenderAPI.GetSampleCount() > VK_SAMPLE_COUNT_1_BIT) {
        VkPipelineMultisampleStateCreateInfo msaaMultisampling = {};
        msaaMultisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        msaaMultisampling.sampleShadingEnable = VK_FALSE;
        msaaMultisampling.rasterizationSamples = vulkanRenderAPI.GetSampleCount();
        vulkanRenderAPI.SetPipelineMultiSampleState(pipelineInfoId, msaaMultisampling);
        vulkanRenderAPI.CreateVulkanPipeline(msaaPipelineID, "forwardMSAAPipeline", pipelineInfoId, "forwardMSAA", 0,
                                             {dynamicDescriptorSetLayout, textureDescriptorSetLayout},
                                             2, 6);
    }
}

void BaseScene::PrepareResources(FrameWork::Camera& camera) {
    auto& renderAPI = vulkanRenderAPI;
    VkRenderPass renderPass = vulkanRenderAPI.GetRenderPass("forward");

    uint32_t colorAttachIdx = -1, depthAttachIdx = -1;
    vulkanRenderAPI.CreateAttachment(colorAttachIdx, vulkanRenderAPI.GetFrameWidth(),
                                     vulkanRenderAPI.GetFrameHeight(), AttachmentType::Color, VK_SAMPLE_COUNT_1_BIT,
                                     true);
    vulkanRenderAPI.CreateAttachment(depthAttachIdx, vulkanRenderAPI.GetFrameWidth(),
                                     vulkanRenderAPI.GetFrameHeight(), AttachmentType::Depth, VK_SAMPLE_COUNT_1_BIT,
                                     false);
    std::vector<uint32_t> attachments = {colorAttachIdx, depthAttachIdx};
    vulkanRenderAPI.CreateFrameBuffer(frameBufferID, attachments, vulkanRenderAPI.GetFrameWidth(),
                                      vulkanRenderAPI.GetFrameHeight(), renderPass);

    uint32_t msaaColorAttachIdx = -1, msaaDepthAttachIdx = -1;
    vulkanRenderAPI.CreateAttachment(msaaColorAttachIdx, vulkanRenderAPI.GetFrameWidth(),
                             vulkanRenderAPI.GetFrameHeight(), AttachmentType::Color, vulkanRenderAPI.GetSampleCount(),
                             false);
    vulkanRenderAPI.CreateAttachment(msaaDepthAttachIdx, vulkanRenderAPI.GetFrameWidth(),
                                     vulkanRenderAPI.GetFrameHeight(), AttachmentType::Depth, vulkanRenderAPI.GetSampleCount(),
                                     false);
    vulkanRenderAPI.CreateFrameBuffer(msaaFrameBufferID, {msaaColorAttachIdx, msaaDepthAttachIdx, colorAttachIdx}, vulkanRenderAPI.GetFrameWidth(),
                                      vulkanRenderAPI.GetFrameHeight(), vulkanRenderAPI.GetRenderPass("forwardMSAA"));
    presentColorAttachment = colorAttachIdx;
    aabbDeBugging.Init("aabbDebug", colorAttachIdx);

    struct GlobalParameter {
        glm::mat4 view = glm::mat4(1.0f);
        glm::mat4 projection = glm::mat4(1.0f);
        void Update(const FrameWork::Camera& camera) {
            view = camera.GetViewMatrix();
            projection = glm::perspective(glm::radians(camera.Zoom),
                                              (float) vulkanRenderAPI.windowWidth / (float) vulkanRenderAPI.windowHeight,
                                              0.01f, 100.0f);
            projection[1][1] *= -1;
        }
    };


    cameraPtr = &camera;
    auto globalSlot = vulkanRenderAPI.CreateSlot(globalSlotID);
    globalSlot->SetUniformObject<GlobalParameter>(
        VK_SHADER_STAGE_VERTEX_BIT, *cameraPtr
        );

    uint32_t modelID_ = -1;
    vulkanRenderAPI.LoadModel(modelID_, "cocona", ModelType::OBJ, DiffuseColor, {0,0, 0}, 1.0f);
    aabbDeBugging.GenerateAABB(modelID_);
    modelID.push_back(modelID_);
    vulkanRenderAPI.GenFace(modelID_, {0, 0, -1}, 1, 1, "../resources/Pic/doro.png");
    aabbDeBugging.GenerateAABB(modelID_);
    modelID.push_back(modelID_);
    sceneName = "Base Scene";

    GUIFunc = [this]() {
        ImGui::Checkbox("MSAA", &useMSAA);
        ImGui::Checkbox("AABB Debug", &displayAABB);
    };
}
