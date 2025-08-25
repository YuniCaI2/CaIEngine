//
// Created by 51092 on 25-8-23.
//

#include "LTCScene.h"
#include <vulkanFrameWork.h>
#include <array>

LTCScene::LTCScene(FrameWork::Camera *camera) {
    auto& api = vulkanRenderAPI;
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

    auto dynamicDescriptorSetLayout = FrameWork::Slot::CreateUniformDescriptorSetLayout(VK_SHADER_STAGE_VERTEX_BIT);
    auto fragDynamicDescriptorSetLayout = FrameWork::Slot::CreateUniformDescriptorSetLayout(VK_SHADER_STAGE_FRAGMENT_BIT);
    auto vertFragDynamicDescriptorSetLayout = FrameWork::Slot::CreateUniformDescriptorSetLayout(VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT);
    auto textureDescriptorSetLayout = FrameWork::Slot::CreateTextureDescriptorSetLayout(VK_SHADER_STAGE_FRAGMENT_BIT);
    vulkanRenderAPI.CreateVulkanPipeline(pipelineID_, "forwardPipeline", pipelineInfoId, "forward", 0,
                                         {dynamicDescriptorSetLayout,
                                             vertFragDynamicDescriptorSetLayout,
                                             dynamicDescriptorSetLayout,
                                             textureDescriptorSetLayout}
                                         );

    //创建floorRenderPass
    auto swapChain = api.GetVulkanSwapChain();
    auto depthFormat = api.GetDepthFormat();
    auto device = api.GetVulkanDevice()->logicalDevice;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    std::array<VkAttachmentDescription, 2> attachments = {};
    // Color attachment
    attachments[0].format = swapChain.colorFormat;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    // Depth attachment
    attachments[1].format = depthFormat;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout =  VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


    VkAttachmentReference colorReference = {};
    colorReference.attachment = 0;
    colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthReference = {};
    depthReference.attachment = 1;
    depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorReference;
    subpassDescription.pDepthStencilAttachment = &depthReference;
    subpassDescription.inputAttachmentCount = 0;
    subpassDescription.pInputAttachments = nullptr;
    subpassDescription.preserveAttachmentCount = 0;
    subpassDescription.pPreserveAttachments = nullptr;
    subpassDescription.pResolveAttachments = nullptr;

    // Subpass dependencies for layout transitions
    std::array<VkSubpassDependency, 2> dependencies{};
    //Access Mask 会指向附件类型

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    dependencies[0].dependencyFlags = 0;

    dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].dstSubpass = 0;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].srcAccessMask = 0;
    dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
    dependencies[1].dependencyFlags = 0;



    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpassDescription;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));

    api.RegisterRenderPass(renderPass, "floorRenderPass");


    vulkanRenderAPI.LoadPipelineShader(pipelineInfoId, "LTCFace", VK_SHADER_STAGE_VERTEX_BIT);
    vulkanRenderAPI.LoadPipelineShader(pipelineInfoId, "LTCFace", VK_SHADER_STAGE_FRAGMENT_BIT);



    vulkanRenderAPI.CreateVulkanPipeline(floorPipelineID_, "forwardPipeline", pipelineInfoId, "forward", 0,
                                     {
                                         dynamicDescriptorSetLayout,
                                         vertFragDynamicDescriptorSetLayout,
                                         fragDynamicDescriptorSetLayout,
                                         textureDescriptorSetLayout,
                                         textureDescriptorSetLayout,
                                         dynamicDescriptorSetLayout
                                     }
                                     );

    //framebuffer
    uint32_t depthAttachment = -1;

    api.CreateAttachment(depthAttachment, api.windowWidth, api.windowHeight, AttachmentType::Depth, VK_SAMPLE_COUNT_1_BIT, false);
    api.CreateAttachment(presentColorAttachment_, api.windowWidth, api.windowHeight, AttachmentType::Color, VK_SAMPLE_COUNT_1_BIT, true);
    api.CreateFrameBuffer(frameBufferID_, {presentColorAttachment_, depthAttachment}, api.windowWidth, api.windowHeight, api.GetRenderPass("forward"));

    //UI
    GUIFunc = [this] {
        ImGui::SliderFloat("Intensity", &intensity_, 0.0f, 10.0f);
        ImGui::SliderFloat("ScaleX", &lightScaleX, 0.0f, 10.0f);
        ImGui::SliderFloat("ScaleY", &lightScaleY, 0.0f, 10.0f);
        ImGui::SliderFloat("RotateX", &lightRotateX, 0.0f, 360.0f);
        ImGui::SliderFloat("RotateY", &lightRotateY, 0.0f, 360.0f);
        ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f);
        ImGui::ColorEdit3("Diffuse", &diffuse[0]);
    };

    //Parameter
    struct GlobalParameter {
        glm::mat4 view = glm::mat4(1.0f);
        glm::mat4 projection = glm::mat4(1.0f);
        void Update(const FrameWork::Camera* camera) {
            view = camera->GetViewMatrix();
            projection = glm::perspective(glm::radians(camera->Zoom),
                                              (float) vulkanRenderAPI.windowWidth / (float) vulkanRenderAPI.windowHeight,
                                                  0.01f, 100.0f);
            projection[1][1] *= -1;
        }
    };

    auto slot = api.CreateSlot(slot_);
    slot->inUse = true;
    slot->SetUniformObject<GlobalParameter>(VK_SHADER_STAGE_VERTEX_BIT , camera);

    //model
    api.GenFace(lightID, {0, 0.5, 0}, {0, 0, 1}, 1, 1, "../resources/Pic/doro.png");
    api.GenFace(floorID, {0, 0.0, 0}, {0, 1, 0}, 20, 20);

    //Light
    auto lightSlot = api.CreateSlot(lightSlot_);
    lightSlot->inUse = true;
    lightSlot->SetUniformObject<Light>(VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT, &lightPos, &lightRotateY, &lightRotateX, &lightScaleY, &lightScaleX, &intensity_, &lightColor, camera_);

    //LTC Loader
    api.CreateTexture(LTCTex1ID_, FrameWork::Resource::GetInstance().LoadTextureFullData("../resources/Pic/LTCMap/ltc_1.dds", SFLOAT16));
    api.CreateTexture(LTCTex2ID_, FrameWork::Resource::GetInstance().LoadTextureFullData("../resources/Pic/LTCMap/ltc_2.dds", SFLOAT16));
    auto ltcSlot = api.CreateSlot(LTCSlot_);
    ltcSlot->inUse = true;
    ltcSlot->SetTexture(VK_SHADER_STAGE_FRAGMENT_BIT,LTCTex1ID_);
    ltcSlot->SetTexture(VK_SHADER_STAGE_FRAGMENT_BIT,LTCTex2ID_);

    //Material
    auto materialSlot = api.CreateSlot(materialSlot_);
    materialSlot->inUse = true;
    materialSlot->SetUniformObject<Material>(VK_SHADER_STAGE_FRAGMENT_BIT, &diffuse, &F0, &roughness);

}

LTCScene::~LTCScene() {
}

void LTCScene::Render(const VkCommandBuffer &cmdBuffer) {
    auto& api = vulkanRenderAPI;
    api.BeginRenderPass("forward", frameBufferID_, api.GetFrameWidth(), api.GetFrameHeight());
    auto vulkanPipeline = api.getByIndex<FrameWork::VulkanPipeline>(pipelineID_);
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline->pipeline);
    auto slot = api.getByIndex<FrameWork::Slot>(slot_);
    auto lightSlot = api.getByIndex<FrameWork::Slot>(lightSlot_);
    slot->Bind(cmdBuffer, vulkanPipeline->pipelineLayout,0);
    lightSlot->Bind(cmdBuffer, vulkanPipeline->pipelineLayout,slot->GetDescriptorSetsSize());
    api.DrawModel(lightID, cmdBuffer, vulkanPipeline->pipelineLayout,  slot->GetDescriptorSetsSize() + lightSlot->GetDescriptorSetsSize());
    api.EndRenderPass();
    api.BeginRenderPass("floorRenderPass", frameBufferID_, api.GetFrameWidth(), api.GetFrameHeight());
    vulkanPipeline = api.getByIndex<FrameWork::VulkanPipeline>(floorPipelineID_);
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline->pipeline);
    auto materialSlot = api.getByIndex<FrameWork::Slot>(materialSlot_);
    auto ltcSlot = api.getByIndex<FrameWork::Slot>(LTCSlot_);
    slot->Bind(cmdBuffer, vulkanPipeline->pipelineLayout,0);
    lightSlot->Bind(cmdBuffer, vulkanPipeline->pipelineLayout,slot->GetDescriptorSetsSize());
    materialSlot->Bind(cmdBuffer, vulkanPipeline->pipelineLayout,slot->GetDescriptorSetsSize() + lightSlot->GetDescriptorSetsSize());
    ltcSlot->Bind(cmdBuffer, vulkanPipeline->pipelineLayout,slot->GetDescriptorSetsSize() + lightSlot->GetDescriptorSetsSize() + materialSlot->GetDescriptorSetsSize());
    api.DrawModel(floorID, cmdBuffer, vulkanPipeline->pipelineLayout,  slot->GetDescriptorSetsSize() + lightSlot->GetDescriptorSetsSize() + materialSlot->GetDescriptorSetsSize() + ltcSlot->GetDescriptorSetsSize());
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
