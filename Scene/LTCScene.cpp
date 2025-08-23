//
// Created by 51092 on 25-8-23.
//

#include "LTCScene.h"
#include <vulkanFrameWork.h>

void LTCScene::UniformBufferObject::Update(const FrameWork::Camera &camera, float& intensity) {
    view = camera.GetViewMatrix();
    projection = glm::perspective(glm::radians(camera.Zoom),
                                  (float)vulkanRenderAPI.windowWidth / (float)vulkanRenderAPI.windowHeight,
                                  0.1f, 100.0f);
    model = glm::mat4(1.0f);
    projection[1][1] *= -1;
    this->intensity = intensity;
}

LTCScene::LTCScene(FrameWork::Camera *camera) {
    camera_ = camera;


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
    vulkanRenderAPI.LoadPipelineShader(pipelineInfoId, "LTCLight", VK_SHADER_STAGE_VERTEX_BIT);
    vulkanRenderAPI.LoadPipelineShader(pipelineInfoId, "LTCLight", VK_SHADER_STAGE_FRAGMENT_BIT);
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
    rasterizer.cullMode = VK_CULL_MODE_NONE;
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

    auto dynamicDescriptorSetLayout = FrameWork::Slot::CreateUniformDescriptorSetLayout(VK_SHADER_STAGE_VERTEX_BIT);
    auto textureDescriptorSetLayout = FrameWork::Slot::CreateTextureDescriptorSetLayout(VK_SHADER_STAGE_FRAGMENT_BIT);
    vulkanRenderAPI.CreateVulkanPipeline(pipelineID_, "forwardPipeline", pipelineInfoId, "forward", 0,
                                         {dynamicDescriptorSetLayout,textureDescriptorSetLayout},
                                         2, 2);


    //framebuffer
    auto& api = vulkanRenderAPI;
    uint32_t depthAttachment = -1;

    api.CreateAttachment(depthAttachment, api.windowWidth, api.windowHeight, AttachmentType::Depth, VK_SAMPLE_COUNT_1_BIT, false);
    api.CreateAttachment(presentColorAttachment_, api.windowWidth, api.windowHeight, AttachmentType::Color, VK_SAMPLE_COUNT_1_BIT, true);
    api.CreateFrameBuffer(frameBufferID_, {presentColorAttachment_, depthAttachment}, api.windowWidth, api.windowHeight, api.GetRenderPass("forward"));

    //UI
    GUIFunc = [this] {
        ImGui::SliderFloat("Intensity", &intensity_, 0.0f, 10.0f);
    };

    //Parameter
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

    auto slot = api.CreateSlot(slot_);
    slot->inUse = true;
    slot->SetUniformObject<GlobalParameter>(VK_SHADER_STAGE_VERTEX_BIT, *camera);

    //model
    api.GenFace(floorID, {0.0f, 0.1f, 0.0f}, 30, 30, "../resources/Pic/doro.png");

}

LTCScene::~LTCScene() {
}

void LTCScene::Render(const VkCommandBuffer &cmdBuffer) {
    auto& api = vulkanRenderAPI;
    api.BeginRenderPass("forward", frameBufferID_, api.GetFrameWidth(), api.GetFrameHeight());
    auto vulkanPipeline = api.getByIndex<FrameWork::VulkanPipeline>(pipelineID_);
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline->pipeline);
    api.getByIndex<FrameWork::Slot>(slot_)->Bind(cmdBuffer, vulkanPipeline->pipelineLayout, 0);
    api.DrawModel(floorID, cmdBuffer, vulkanPipeline->pipelineLayout,  api.getByIndex<FrameWork::Slot>(slot_)->GetDescriptorSetsSize());
    api.EndRenderPass();
}

const std::function<void()> & LTCScene::GetRenderFunction() {
    return GUIFunc;
}

std::vector<uint32_t> LTCScene::GetModelIDs() {
    return {};
}

std::string LTCScene::GetName() const {
    return "LTCScene";
}

uint32_t LTCScene::GetPresentColorAttachment() {
    return presentColorAttachment_;
}
