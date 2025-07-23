//
// Created by AI Assistant on 25-5-26.
//
#include <vulkanFrameWork.h>
#include "VulkanWindow.h"
#define _VALIDATION 1

class TriangleRenderer {
private:
    struct UniformBufferObject {
        glm::mat4 projection;
        glm::mat4 model;
        glm::mat4 view;
    };

    uint32_t meshID = -1;
    uint32_t pipelineID = -1;
    uint32_t materialID = -1;
    uint32_t frameBufferID = -1;
    uint32_t modelID = -1;
    VkDescriptorSetLayout dynamicDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout textureDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;
    UniformBufferObject ubo{};
    // std::vector<FrameWork::Vertex> vertices = {
    //     {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}},
    //     {{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}},
    //     {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
    //     {{-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 0.0f}},
    //
    //     {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}},
    //     {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}},
    //     {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}},
    //     {{-0.5f,  0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}}
    // };
    // std::vector<uint32_t> indices = {
    //     0, 1, 2,   2, 3, 0,
    //     4, 6, 5,   6, 4, 7,
    //     4, 0, 3,   3, 7, 4,
    //     1, 5, 6,   6, 2, 1,
    //     3, 2, 6,   6, 7, 3,
    //     4, 5, 1,   1, 0, 4
    // };
public:
    TriangleRenderer() {
        vulkanRenderAPI.SetTitle("Triangle Renderer");
        vulkanRenderAPI.GetCamera().Position = glm::vec3(0.0f, 0.0f, 3.0f);
    }
    ~TriangleRenderer() {
        // 清理资源
        if (vulkanRenderAPI.GetVulkanDevice()->logicalDevice) {
            // vkDestroyPipeline(device, graphicsPipeline, nullptr);
            // vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

            // vertexBuffer.destroy();
            // indexBuffer.destroy();
        }
    }
    void buildCommandBuffers() {
        VkCommandBufferBeginInfo cmdBufInfo = {};
        cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        VkClearValue clearValues[2];
        clearValues[0].color = vulkanRenderAPI.defaultClearColor;
        clearValues[1].depthStencil = {1.0f, 0};
        VkRenderPassBeginInfo renderPassBeginInfo = {};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass = vulkanRenderAPI.GetRenderPass("forward");
        renderPassBeginInfo.renderArea.offset.x = 0;
        renderPassBeginInfo.renderArea.offset.y = 0;
        renderPassBeginInfo.renderArea.extent.width = vulkanRenderAPI.GetFrameWidth();
        renderPassBeginInfo.renderArea.extent.height = vulkanRenderAPI.GetFrameHeight();
        renderPassBeginInfo.clearValueCount = 2;
        renderPassBeginInfo.pClearValues = clearValues;
        renderPassBeginInfo.framebuffer = vulkanRenderAPI.getByIndex<FrameWork::VulkanFBO>(frameBufferID)->framebuffers[vulkanRenderAPI.GetCurrentFrame()];
        VK_CHECK_RESULT(vkBeginCommandBuffer(vulkanRenderAPI.GetCurrentCommandBuffer(), &cmdBufInfo));
        vkCmdBeginRenderPass(vulkanRenderAPI.GetCurrentCommandBuffer(), &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        VkViewport viewport = {};
        viewport.width = (float)vulkanRenderAPI.windowWidth;
        viewport.height = (float)vulkanRenderAPI.windowHeight;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(vulkanRenderAPI.GetCurrentCommandBuffer(), 0, 1, &viewport);
        VkRect2D scissor = {};
        scissor.extent.width = vulkanRenderAPI.windowWidth;
        scissor.extent.height = vulkanRenderAPI.windowHeight;
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        vkCmdSetScissor(vulkanRenderAPI.GetCurrentCommandBuffer(), 0, 1, &scissor);
        vkCmdBindPipeline(vulkanRenderAPI.GetCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
        vulkanRenderAPI.DrawModel(modelID, vulkanRenderAPI.GetCurrentCommandBuffer(), pipelineLayout);
        vkCmdEndRenderPass(vulkanRenderAPI.GetCurrentCommandBuffer());
        vulkanRenderAPI.PresentFrame(vulkanRenderAPI.GetCurrentCommandBuffer(), vulkanRenderAPI.GetCurrentImageIndex());
        VK_CHECK_RESULT(vkEndCommandBuffer(vulkanRenderAPI.GetCurrentCommandBuffer()));
    }
    void updateUniformBuffer() {
        ubo.model = glm::mat4(1.0f);
        ubo.view = vulkanRenderAPI.GetCamera().GetViewMatrix();
        ubo.projection = glm::perspective(glm::radians(vulkanRenderAPI.GetCamera().Zoom), (float)vulkanRenderAPI.windowWidth / (float)vulkanRenderAPI.windowHeight, 0.1f, 100.0f);
        ubo.projection[1][1] *= -1;
        for (auto& m : (vulkanRenderAPI.getByIndex<FrameWork::Model>(modelID)->materials)) {
            auto material = vulkanRenderAPI.getByIndex<FrameWork::Material>(m);
            for (uint32_t i = 0; i < material->uniformBuffer.size(); i++) {
                auto u = material->uniformBuffer[i];
                vulkanRenderAPI.UpdateUniformBuffer({u},{&ubo},{sizeof(decltype(ubo))}, vulkanRenderAPI.currentFrame * material->uniformBufferSizes[i] / vulkanRenderAPI.MaxFrame);
                //注意这里的UniformBuffer指的就是整个UniformBuffer的大小
            }
        }
    }
    void createDescriptorSetLayout() {
        dynamicDescriptorSetLayout = vulkanRenderAPI.CreateDescriptorSetLayout(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_VERTEX_BIT);
        textureDescriptorSetLayout =  vulkanRenderAPI.CreateDescriptorSetLayout(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                1, VK_SHADER_STAGE_FRAGMENT_BIT);

    }
    void createDescriptorSet() {
        ubo.model = glm::mat4(1.0f);
        ubo.view = vulkanRenderAPI.GetCamera().GetViewMatrix();
        ubo.projection = glm::perspective(glm::radians(vulkanRenderAPI.GetCamera().Zoom), (float)vulkanRenderAPI.windowWidth / (float)vulkanRenderAPI.windowHeight, 0.1f, 100.0f);
        FrameWork::MaterialCreateInfo materialInfo = {
            .UniformDescriptorLayouts = {dynamicDescriptorSetLayout}, //设置uniform
            .TexturesDescriptorLayouts = {},
            .UniformData = {{&ubo, sizeof(UniformBufferObject)}},
            .TexturesDatas = {}
        };
        vulkanRenderAPI.CreateMaterial(materialID, materialInfo);
    }
    void createGraphicsPipeline() {
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
        vulkanRenderAPI.CreateVulkanPipeline(pipelineID, "forwardPipeline", pipelineInfoId, "forward", 0, {dynamicDescriptorSetLayout, textureDescriptorSetLayout});
        graphicsPipeline = vulkanRenderAPI.getByIndex<FrameWork::VulkanPipeline>(pipelineID)->pipeline;
        pipelineLayout = vulkanRenderAPI.getByIndex<FrameWork::VulkanPipeline>(pipelineID)->pipelineLayout;
    }
    void prepare() {
        createDescriptorSetLayout();
        VkRenderPass renderPass = vulkanRenderAPI.GetRenderPass("forward");
        createGraphicsPipeline();
        ubo.model = glm::mat4(1.0f);
        ubo.view = vulkanRenderAPI.GetCamera().GetViewMatrix();
        ubo.projection = glm::perspective(glm::radians(vulkanRenderAPI.GetCamera().Zoom), (float)vulkanRenderAPI.windowWidth / (float)vulkanRenderAPI.windowHeight, 0.1f, 100.0f);
        FrameWork::MaterialCreateInfo materialInfo = {
            .UniformDescriptorLayouts = {dynamicDescriptorSetLayout},
            .TexturesDescriptorLayouts = {textureDescriptorSetLayout},
            .UniformData = {{&ubo, sizeof(UniformBufferObject)}},
            .TexturesDatas = {}//留给LoadModel填写
        };
        //Material包含在模型中
        vulkanRenderAPI.LoadModel(modelID, "cocona", materialInfo);

        // createDescriptorSet();
        uint32_t colorAttachIdx = -1, depthAttachIdx = -1;
        vulkanRenderAPI.CreateAttachment(colorAttachIdx, vulkanRenderAPI.GetFrameWidth(), vulkanRenderAPI.GetFrameHeight(), AttachmentType::Color, VK_SAMPLE_COUNT_1_BIT, true);
        vulkanRenderAPI.CreateAttachment(depthAttachIdx, vulkanRenderAPI.GetFrameWidth(), vulkanRenderAPI.GetFrameHeight(), AttachmentType::Depth, VK_SAMPLE_COUNT_1_BIT, false);
        std::vector<uint32_t> attachments = {colorAttachIdx, depthAttachIdx};
        vulkanRenderAPI.CreateFrameBuffer(frameBufferID, attachments, vulkanRenderAPI.GetFrameWidth(), vulkanRenderAPI.GetFrameHeight(), renderPass);
        vulkanRenderAPI.InitPresent("uniformPresent", colorAttachIdx);
    }
    void render() {
        updateUniformBuffer();
        vulkanRenderAPI.prepareFrame();
        buildCommandBuffers();
        vulkanRenderAPI.submitFrame();
    }
};

int main() {
    FrameWork::Locator::RegisterService<FrameWork::InputManager>(std::make_shared<FrameWork::InputManager>(FrameWork::VulkanWindow::GetInstance().GetWindow()));
    FrameWork::Locator::RegisterService<FrameWork::Resource>(std::make_shared<FrameWork::Resource>());
    TriangleRenderer app;
    vulkanRenderAPI.initVulkan();
    app.prepare();
    WINDOW_LOOP(
        vulkanRenderAPI.Update();
        app.render();)
    return 0;
}
