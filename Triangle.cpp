//
// Created by AI Assistant on 25-5-26.
//
#include <vulkanFrameWork.h>

#include "VulkanWindow.h"
#define _VALIDATION 1

class TriangleRenderer : public vulkanFrameWork {
private:
    // 顶点数据结构


    // UBO数据结构
    struct UniformBufferObject {
        glm::mat4 projection;
        glm::mat4 model;
        glm::mat4 view;
    };

    // 顶点和索引缓冲区
    // FrameWork::Buffer vertexBuffer;
    // FrameWork::Buffer indexBuffer;
    uint32_t meshID = -1;
    uint32_t pipelineID = -1;
    uint32_t materialID = -1;
    uint32_t frameBufferID = -1;

    FrameWork::Buffer uniformBuffer;

    // 描述符集相关
    VkDescriptorSetLayout dynamicDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;

    UniformBufferObject ubo{};
    // 立方体顶点数据
    std::vector<FrameWork::Vertex> vertices = {
        // 前面 (Z = 0.5)
        {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}}, // 左下
        {{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}}, // 右下
        {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}}, // 右上
        {{-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 0.0f}}, // 左上

        // 后面 (Z = -0.5)
        {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}}, // 左下
        {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}}, // 右下
        {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}}, // 右上
        {{-0.5f,  0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}}  // 左上
    };

    // 立方体索引数据（定义12个三角形组成6个面）
    std::vector<uint32_t> indices = {
        // 前面
        0, 1, 2,   2, 3, 0,

        // 后面
        4, 6, 5,   6, 4, 7,

        // 左面
        4, 0, 3,   3, 7, 4,

        // 右面
        1, 5, 6,   6, 2, 1,

        // 上面
        3, 2, 6,   6, 7, 3,

        // 下面
        4, 5, 1,   1, 0, 4
    };

public:
    TriangleRenderer() {
        title = "Triangle Renderer";
        camera.Position = glm::vec3(0.0f, 0.0f, 3.0f);
    }

    ~TriangleRenderer() {
        // 清理资源
        if (device) {
            // vkDestroyPipeline(device, graphicsPipeline, nullptr);
            // vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

            // vertexBuffer.destroy();
            // indexBuffer.destroy();
            uniformBuffer.destroy();
        }
    }

    void buildCommandBuffers() override {
        VkCommandBufferBeginInfo cmdBufInfo = {};
        cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VkClearValue clearValues[2];
        clearValues[0].color = defaultClearColor;
        clearValues[1].depthStencil = {1.0f, 0};

        VkRenderPassBeginInfo renderPassBeginInfo = {};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass = renderPass;
        renderPassBeginInfo.renderArea.offset.x = 0;
        renderPassBeginInfo.renderArea.offset.y = 0;
        renderPassBeginInfo.renderArea.extent.width = windowWidth;
        renderPassBeginInfo.renderArea.extent.height = windowHeight;
        renderPassBeginInfo.clearValueCount = 2;
        renderPassBeginInfo.pClearValues = clearValues;

        renderPassBeginInfo.framebuffer = frameBuffers[GetCurrentFrame()];

        VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[currentFrame], &cmdBufInfo));

        vkCmdBeginRenderPass(drawCmdBuffers[currentFrame], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport = {};
        viewport.width = (float) windowWidth;
        viewport.height = (float) windowHeight;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(drawCmdBuffers[currentFrame], 0, 1, &viewport);

        VkRect2D scissor = {};
        scissor.extent.width = windowWidth;
        scissor.extent.height = windowHeight;
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        vkCmdSetScissor(drawCmdBuffers[currentFrame], 0, 1, &scissor);

        // 绑定图形管线
        vkCmdBindPipeline(drawCmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        // // 绑定描述符集
        // vkCmdBindDescriptorSets(drawCmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
        //                         &descriptorSet, 0, nullptr);
        std::vector descriptorOffset = {
            currentFrame * getByIndex<FrameWork::Material>(materialID)->uniformBufferSizes[0] / MAX_FRAME
        };
        vkCmdBindDescriptorSets(drawCmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                                &getByIndex<FrameWork::Material>(materialID)->descriptorPairs[0].first, descriptorOffset.size(), descriptorOffset.data());

        // 绑定顶点缓冲区
        VkBuffer vertexBuffers[] = {getByIndex<FrameWork::Mesh>(meshID)->VertexBuffer.buffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(drawCmdBuffers[currentFrame], 0, 1, vertexBuffers, offsets);

        // 绑定索引缓冲区
        vkCmdBindIndexBuffer(drawCmdBuffers[currentFrame], getByIndex<FrameWork::Mesh>(meshID)->IndexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        // 绘制三角形
        vkCmdDrawIndexed(drawCmdBuffers[currentFrame], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);


        vkCmdEndRenderPass(drawCmdBuffers[currentFrame]);

        PresentFrame(drawCmdBuffers[currentFrame], imageIndex);

        VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[currentFrame]));
    }

    void createFrameBuffer(){
        uint32_t colorAttachment = -1, depthAttachment = -1;
    }

    void createUniformBuffer() {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        VK_CHECK_RESULT(vulkanDevice->createBuffer(
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &uniformBuffer,
            bufferSize));

        VK_CHECK_RESULT(uniformBuffer.map());

    }

    void updateUniformBuffer() {
        // 暂时不旋转，让三角形保持静止以便调试
        ubo.model = glm::mat4(1.0f); // 单位矩阵，无变换
        ubo.view = camera.GetViewMatrix();
        ubo.projection = glm::perspective(glm::radians(camera.Zoom), (float)windowWidth / (float)windowHeight, 0.1f, 100.0f);

        uniformBuffer.copyTo(&ubo, sizeof(ubo));

        auto material = getByIndex<FrameWork::Material>(meshID);
        for (uint32_t i = 0; i < material->uniformBuffer.size(); i++) {
            auto u = material->uniformBuffer[i];
            UpdateUniformBuffer({u},{&ubo},{sizeof(decltype(ubo))}, currentFrame * material->uniformBufferSizes[i] / MAX_FRAME);
            //注意这里的UniformBuffer指的就是整个UniformBuffer的大小
        }
    }

    void createDescriptorSetLayout() {
        dynamicDescriptorSetLayout = CreateDescriptorSetLayout(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_VERTEX_BIT);
    }

    void createDescriptorPool() {
        VkDescriptorPoolSize poolSize = {};
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = 1;

        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = 1;

        VK_CHECK_RESULT(vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool));
    }

    void createDescriptorSet() {
        //测试Material接口
        // 暂时不旋转，让三角形保持静止以便调试
        ubo.model = glm::mat4(1.0f); // 单位矩阵，无变换
        ubo.view = camera.GetViewMatrix();
        ubo.projection = glm::perspective(glm::radians(camera.Zoom), (float)windowWidth / (float)windowHeight, 0.1f, 100.0f);

        FrameWork::MaterialCreateInfo materialInfo = {
            .UniformDescriptorLayouts = {dynamicDescriptorSetLayout},
            .TexturesDescriptorLayouts = {},
            .UniformData = {{&ubo, sizeof(UniformBufferObject)}},
            .TexturesDatas = {}
        };
        CreateMaterial(materialID, materialInfo);
    }

    void createGraphicsPipeline() {
        // 顶点输入状态
        VkVertexInputBindingDescription bindingDescription = FrameWork::Vertex::getBindingDescription();
        auto attributeDescriptions = FrameWork::Vertex::getAttributeDescription();

        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = 2;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        // 输入装配状态
        VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // 视口状态
        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float) windowWidth;
        viewport.height = (float) windowHeight;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor = {};
        scissor.offset = {0, 0};
        scissor.extent = {windowWidth, windowHeight};

        VkPipelineViewportStateCreateInfo viewportState = {};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        // 光栅化状态
        VkPipelineRasterizationStateCreateInfo rasterizer = {};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        // 多重采样状态
        VkPipelineMultisampleStateCreateInfo multisampling = {};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // 深度模板状态
        VkPipelineDepthStencilStateCreateInfo depthStencil = {};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        // 颜色混合状态
        VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending = {};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        // 管线布局


        uint32_t pipelineInfoId = -1;
        InitPipelineInfo(pipelineInfoId);
        LoadPipelineShader(pipelineInfoId, "triangle", VK_SHADER_STAGE_VERTEX_BIT);
        LoadPipelineShader(pipelineInfoId, "triangle", VK_SHADER_STAGE_FRAGMENT_BIT);
        AddPipelineVertexAttributeDescription(pipelineInfoId, attributeDescriptions);
        AddPipelineVertexBindingDescription(pipelineInfoId, bindingDescription);
        SetPipelineViewPort(pipelineInfoId, viewport);
        SetPipelineScissor(pipelineInfoId, scissor);
        SetPipelineRasterizationState(pipelineInfoId, rasterizer);
        SetPipelineDepthStencilState(pipelineInfoId, depthStencil);
        SetPipelineMultiSampleState(pipelineInfoId, multisampling);
        AddPipelineColorBlendState(pipelineInfoId, true, BlendOp::Opaque);
        CreateVulkanPipeline(pipelineID, "forwardPipeline", pipelineInfoId, "forward", 0, {dynamicDescriptorSetLayout});

        graphicsPipeline = getByIndex<FrameWork::VulkanPipeline>(pipelineID)->pipeline;
        pipelineLayout = getByIndex<FrameWork::VulkanPipeline>(pipelineID)->pipelineLayout;
    }

    void prepare() override {
        vulkanFrameWork::prepare();

        SetUpStaticMesh(meshID, vertices, indices, false);
        createDescriptorSetLayout();
        createUniformBuffer();
        RegisterRenderPass(renderPass, "forward");
        createDescriptorPool(); // 添加这行：在创建描述符集之前先创建描述符池
        createDescriptorSet();
        createGraphicsPipeline();
        uint32_t colorAttachIdx = -1, depthAttachIdx = -1;
        CreateAttachment(colorAttachIdx, GetFrameWidth(), GetFrameHeight(), AttachmentType::Color, VK_SAMPLE_COUNT_1_BIT, true);
        CreateAttachment(depthAttachIdx, GetFrameWidth(), GetFrameHeight(), AttachmentType::Depth, VK_SAMPLE_COUNT_1_BIT, false);
        std::vector attachments = {
            colorAttachIdx, depthAttachIdx
        };
        CreateFrameBuffer(frameBufferID, attachments, renderPass);
        frameBuffers = getByIndex<FrameWork::VulkanFBO>(frameBufferID)->framebuffers;
        InitPresent("uniformPresent", colorAttachIdx);

    }

    void render() override {
        updateUniformBuffer();
        vulkanFrameWork::prepareFrame();
        vulkanFrameWork::submitFrame();
    }
};

// 主函数
int main() {
    //注册定位器
    FrameWork::Locator::RegisterService<FrameWork::InputManager>
    (std::make_shared<FrameWork::InputManager>(FrameWork::VulkanWindow::GetInstance().GetWindow())
        );
    FrameWork::Locator::RegisterService<FrameWork::Resource>(
        std::make_shared<FrameWork::Resource>()
        );

    TriangleRenderer app;
    app.initVulkan();
    app.renderLoop();
    return 0;
}
