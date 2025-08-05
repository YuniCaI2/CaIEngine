//
// Created by AI Assistant on 25-5-26.
//
#include <vulkanFrameWork.h>

#include "FrameWorkGUI.h"
#include "Timer.h"
#include "VulkanDebug.h"
#include "VulkanWindow.h"

class Renderer {
private:
    struct UniformBufferObject {
        glm::mat4 projection;
        glm::mat4 model;
        glm::mat4 view;
    };

    uint32_t meshID = -1;
    uint32_t pipelineID = -1;
    uint32_t frameBufferID = -1;
    std::vector<uint32_t> modelID;
    VkDescriptorSetLayout dynamicDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout textureDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;
    UniformBufferObject ubo{};

    FrameWork::Camera camera{};
    FrameWork::Timer timer;
    FrameWork::FrameWorkGUI GUI{};

    FrameWork::AABBDeBugging aabbDeBugging{};
    bool displayAABB = false;

public:
    Renderer() {
        vulkanRenderAPI.SetTitle("Triangle Renderer");
        camera.Position = glm::vec3(0.0f, 0.0f, 3.0f);
    }

    ~Renderer() {
        GUI.ReleaseGUIResources();
        aabbDeBugging.Destroy();
    }

    void buildCommandBuffers() {
        auto cmdBuffer = vulkanRenderAPI.BeginCommandBuffer();
        vulkanRenderAPI.BeginRenderPass("forward", frameBufferID, vulkanRenderAPI.GetFrameWidth(), vulkanRenderAPI.GetFrameHeight());
        vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
        for (int i = 0; i < modelID.size(); i++) {
            vulkanRenderAPI.DrawModel(modelID[i], cmdBuffer, pipelineLayout);
        }
        vulkanRenderAPI.EndRenderPass();
        //---------------------------------------------------------------------------------
        if (displayAABB) {
            aabbDeBugging.Draw(cmdBuffer);
        }
        vulkanRenderAPI.PresentFrame(cmdBuffer, vulkanRenderAPI.GetCurrentImageIndex());
        GUI.RenderGUI(cmdBuffer);

        vulkanRenderAPI.EndCommandBuffer();
    }

    void updateUniformBuffer() {
        ubo.view = camera.GetViewMatrix();
        ubo.projection = glm::perspective(glm::radians(camera.Zoom),
                                          (float) vulkanRenderAPI.windowWidth / (float) vulkanRenderAPI.windowHeight,
                                          0.1f, 100.0f);
        ubo.model = glm::mat4(1.0f);
        ubo.projection[1][1] *= -1;
        aabbDeBugging.Update(ubo.view, ubo.projection);
        for (unsigned int n : modelID) {
            auto model = vulkanRenderAPI.getByIndex<FrameWork::Model>(n);
            ubo.model = glm::translate(ubo.model, model->position);
            for (auto &m: model->materials) {
                auto material = vulkanRenderAPI.getByIndex<FrameWork::Material>(m);
                for (uint32_t i = 0; i < material->uniformBuffer.size(); i++) {
                    auto u = material->uniformBuffer[i];
                    vulkanRenderAPI.UpdateUniformBuffer({u}, {&ubo}, {sizeof(decltype(ubo))},
                                                        vulkanRenderAPI.currentFrame * material->uniformBufferSizes[i] /
                                                        vulkanRenderAPI.MaxFrame);
                    //注意这里的UniformBuffer指的就是整个UniformBuffer的大小
                }
            }
        }
    }

    void createDescriptorSetLayout() {
        dynamicDescriptorSetLayout = vulkanRenderAPI.CreateDescriptorSetLayout(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,  VK_SHADER_STAGE_VERTEX_BIT);
        textureDescriptorSetLayout = vulkanRenderAPI.CreateDescriptorSetLayout(
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
             VK_SHADER_STAGE_FRAGMENT_BIT);
        vulkanRenderAPI.RegisterDescriptorSetLayout(dynamicDescriptorSetLayout, "dynamicDescriptorSetLayout");
        vulkanRenderAPI.RegisterDescriptorSetLayout(textureDescriptorSetLayout, "textureDescriptorSetLayout");
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
        vulkanRenderAPI.CreateVulkanPipeline(pipelineID, "forwardPipeline", pipelineInfoId, "forward", 0,
                                             {dynamicDescriptorSetLayout, textureDescriptorSetLayout},
                                             1, 6); //可以设置MAX_TexNum
        graphicsPipeline = vulkanRenderAPI.getByIndex<FrameWork::VulkanPipeline>(pipelineID)->pipeline;
        pipelineLayout = vulkanRenderAPI.getByIndex<FrameWork::VulkanPipeline>(pipelineID)->pipelineLayout;
    }

    void prepare() {
        createDescriptorSetLayout();
        VkRenderPass renderPass = vulkanRenderAPI.GetRenderPass("forward");
        createGraphicsPipeline();

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
        vulkanRenderAPI.InitPresent("uniformPresent", colorAttachIdx);
        aabbDeBugging.Init("aabbDebug", colorAttachIdx, depthAttachIdx);


        //加载模型
        ubo.model = glm::mat4(1.0f);
        ubo.view = camera.GetViewMatrix();
        ubo.projection = glm::perspective(glm::radians(camera.Zoom),
                                          (float) vulkanRenderAPI.windowWidth / (float) vulkanRenderAPI.windowHeight,
                                          0.1f, 100.0f);
        FrameWork::MaterialCreateInfo materialInfo = {
            .UniformDescriptorLayouts = {dynamicDescriptorSetLayout},
            .TexturesDescriptorLayouts = {textureDescriptorSetLayout},
            .UniformData = {{&ubo, sizeof(UniformBufferObject)}},
            .TexturesDatas = {} //留给LoadModel填写
        };

        //Material包含在模型中
        uint32_t modelID_ = -1;
        vulkanRenderAPI.LoadModel(modelID_, "cocona", ModelType::OBJ, materialInfo, DiffuseColor);
        aabbDeBugging.GenerateAABB(modelID_);
        modelID.push_back(modelID_);

        GUI.InitFrameWorkGUI();
        SetGUI();
    }

    void render() {
        camera.update(timer.GetElapsedSeconds());
        updateUniformBuffer();
        vulkanRenderAPI.prepareFrame(timer.GetElapsedMilliTime());
        buildCommandBuffers();
        vulkanRenderAPI.submitFrame();
        timer.Restart();
    }

    void SetGUI() {
        GUI.SetGUIItems(
            [this] {
                ImGui::Checkbox("AABB", &displayAABB);
            }
        );
    }
};

int main() {
    vulkanRenderAPI.initVulkan(); {
        Renderer app;
        app.prepare();
        auto &inputManager = FrameWork::InputManager::GetInstance();
        WINDOW_LOOP(
            inputManager.update();
            app.render();
        )
    }
    return 0;
}
