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

        void Update() {
            view = renderer->camera.GetViewMatrix();
            projection = glm::perspective(glm::radians(renderer->camera.Zoom),
                                              (float) vulkanRenderAPI.windowWidth / (float) vulkanRenderAPI.windowHeight,
                                              0.1f, 100.0f);
            model = glm::mat4(1.0f);
            projection[1][1] *= -1;
        }

        inline static Renderer* renderer{};
        static void SetRenderer(Renderer* renderer_) {
            renderer = renderer_;
        }
    };

    struct GlobalParameter {
        glm::mat4 view;
        glm::mat4 projection;
    };

    struct MaterialParameter {
        glm::mat4 position;
    };

    uint32_t meshID = -1;
    uint32_t pipelineID = -1;
    uint32_t frameBufferID = -1;
    uint32_t globalSlotID = -1;
    std::vector<uint32_t> modelID;
    VkDescriptorSetLayout dynamicDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout textureDescriptorSetLayout = VK_NULL_HANDLE;

    //MSAA Resource
    uint32_t msaaPipelineID = -1;
    uint32_t msaaFrameBufferID = -1;

    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;
    UniformBufferObject ubo{};

    FrameWork::Slot slotTest{};

    FrameWork::Camera camera{};
    FrameWork::Timer timer;
    FrameWork::FrameWorkGUI GUI{};

    FrameWork::AABBDeBugging aabbDeBugging{};
    bool displayAABB = false;
    bool useMSAA = false;

public:
    friend struct UniformBufferObject;
    Renderer() {
        vulkanRenderAPI.SetTitle("Triangle Renderer");
        camera.Position = glm::vec3(0.0f, 0.0f, 3.0f);
    }

    ~Renderer() {
        GUI.ReleaseGUIResources();
        FrameWork::Slot::DestroyDescriptorSetLayout();
        aabbDeBugging.Destroy();
    }

    void buildCommandBuffers() {
        auto cmdBuffer = vulkanRenderAPI.BeginCommandBuffer();
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
                                             1, 6);
        graphicsPipeline = vulkanRenderAPI.getByIndex<FrameWork::VulkanPipeline>(pipelineID)->pipeline;
        pipelineLayout = vulkanRenderAPI.getByIndex<FrameWork::VulkanPipeline>(pipelineID)->pipelineLayout;

        // 为MSAA创建完全独立的管线信息（如果需要的话）
        if (vulkanRenderAPI.GetSampleCount() > VK_SAMPLE_COUNT_1_BIT) {

            // MSAA特定的多重采样状态
            VkPipelineMultisampleStateCreateInfo msaaMultisampling = {};
            msaaMultisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            msaaMultisampling.sampleShadingEnable = VK_FALSE;
            msaaMultisampling.rasterizationSamples = vulkanRenderAPI.GetSampleCount();
            vulkanRenderAPI.SetPipelineMultiSampleState(pipelineInfoId, msaaMultisampling);
            vulkanRenderAPI.CreateVulkanPipeline(msaaPipelineID, "forwardMSAAPipeline", pipelineInfoId, "forwardMSAA", 0,
                                                 {dynamicDescriptorSetLayout, textureDescriptorSetLayout},
                                                 1, 6);
        }
    }

    void prepare() {
        UniformBufferObject::SetRenderer(this);
        createDescriptorSetLayout();
        VkRenderPass renderPass = vulkanRenderAPI.GetRenderPass("forward");
        createGraphicsPipeline();
        auto& renderAPI = vulkanRenderAPI;
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

        //呈现
        vulkanRenderAPI.InitPresent("uniformPresent", colorAttachIdx);
        //DeBug
        aabbDeBugging.Init("aabbDebug", colorAttachIdx);


        //加载模型
        ubo.model = glm::mat4(1.0f);
        ubo.view = camera.GetViewMatrix();
        ubo.projection = glm::perspective(glm::radians(camera.Zoom),
                                          (float) vulkanRenderAPI.windowWidth / (float) vulkanRenderAPI.windowHeight,
                                          0.1f, 100.0f);
        //加载globalSlot

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
                ImGui::Checkbox("MSAA", &useMSAA);
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
