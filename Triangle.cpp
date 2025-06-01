//
// Created by AI Assistant on 25-5-26.
//
#include <vulkanFrameWork.h>

#include "VulkanWindow.h"
#define _VALIDATION 1

class TriangleRenderer : public vulkanFrameWork {
private:
    // 顶点数据结构
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 color;
    };

    // UBO数据结构
    struct UniformBufferObject {
        glm::mat4 projection;
        glm::mat4 model;
        glm::mat4 view;
    };

    // 顶点和索引缓冲区
    FrameWork::Buffer vertexBuffer;
    FrameWork::Buffer indexBuffer;
    FrameWork::Buffer uniformBuffer;

    // 描述符集相关
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;

    // 立方体顶点数据
    std::vector<Vertex> vertices = {
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
            vkDestroyPipeline(device, graphicsPipeline, nullptr);
            vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
            vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

            vertexBuffer.destroy();
            indexBuffer.destroy();
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
        renderPassBeginInfo.renderArea.extent.width = width;
        renderPassBeginInfo.renderArea.extent.height = height;
        renderPassBeginInfo.clearValueCount = 2;
        renderPassBeginInfo.pClearValues = clearValues;

        renderPassBeginInfo.framebuffer = frameBuffers[imageIndex];

        VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[currentFrame], &cmdBufInfo));

        vkCmdBeginRenderPass(drawCmdBuffers[currentFrame], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport = {};
        viewport.width = (float) width;
        viewport.height = (float) height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(drawCmdBuffers[currentFrame], 0, 1, &viewport);

        VkRect2D scissor = {};
        scissor.extent.width = width;
        scissor.extent.height = height;
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        vkCmdSetScissor(drawCmdBuffers[currentFrame], 0, 1, &scissor);

        // 绑定图形管线
        vkCmdBindPipeline(drawCmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        // 绑定描述符集
        vkCmdBindDescriptorSets(drawCmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                                &descriptorSet, 0, nullptr);

        // 绑定顶点缓冲区
        VkBuffer vertexBuffers[] = {vertexBuffer.buffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(drawCmdBuffers[currentFrame], 0, 1, vertexBuffers, offsets);

        // 绑定索引缓冲区
        vkCmdBindIndexBuffer(drawCmdBuffers[currentFrame], indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        // 绘制三角形
        vkCmdDrawIndexed(drawCmdBuffers[currentFrame], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);


        vkCmdEndRenderPass(drawCmdBuffers[currentFrame]);

        VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[currentFrame]));
    }

    void createVertexBuffer() {
        // 创建顶点缓冲区
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        // 创建暂存缓冲区
        FrameWork::Buffer stagingBuffer;
        VK_CHECK_RESULT(vulkanDevice->createBuffer(
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &stagingBuffer,
            bufferSize,
            vertices.data()));

        // 创建顶点缓冲区
        VK_CHECK_RESULT(vulkanDevice->createBuffer(
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            &vertexBuffer,
            bufferSize)); // 复制数据
        vulkanDevice->copyBuffer(&stagingBuffer, &vertexBuffer, queue);
        stagingBuffer.destroy();
    }

    void createIndexBuffer() {
        // 创建索引缓冲区
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        // 创建暂存缓冲区
        FrameWork::Buffer stagingBuffer;
        VK_CHECK_RESULT(vulkanDevice->createBuffer(
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &stagingBuffer,
            bufferSize,
            indices.data()));

        // 创建索引缓冲区
        VK_CHECK_RESULT(vulkanDevice->createBuffer(
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            &indexBuffer,
            bufferSize)); // 复制数据
        vulkanDevice->copyBuffer(&stagingBuffer, &indexBuffer, queue);
        stagingBuffer.destroy();
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
        UniformBufferObject ubo = {};
        // 暂时不旋转，让三角形保持静止以便调试
        ubo.model = glm::mat4(1.0f); // 单位矩阵，无变换
        ubo.view = camera.GetViewMatrix();
        ubo.projection = glm::perspective(glm::radians(camera.Zoom), (float)width / (float)height, 0.1f, 100.0f);

        uniformBuffer.copyTo(&ubo, sizeof(ubo));
    }

    void createDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding uboLayoutBinding = {};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uboLayoutBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &uboLayoutBinding;

        VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout));
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
        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &descriptorSetLayout;

        VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));

        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = uniformBuffer.buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkWriteDescriptorSet descriptorWrite = {};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSet;
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
    }

    void createGraphicsPipeline() {
        // 加载着色器
        auto vertShaderStageInfo = loadShader(getShaderPath() + "triangle/triangle.vert.spv",
                                              VK_SHADER_STAGE_VERTEX_BIT);
        auto fragShaderStageInfo = loadShader(getShaderPath() + "triangle/triangle.frag.spv",
                                              VK_SHADER_STAGE_FRAGMENT_BIT);

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        // 顶点输入状态
        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        VkVertexInputAttributeDescription attributeDescriptions[2] = {};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = 2;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

        // 输入装配状态
        VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // 视口状态
        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float) width;
        viewport.height = (float) height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor = {};
        scissor.offset = {0, 0};
        scissor.extent = {width, height};

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
        colorBlending.pAttachments = &colorBlendAttachment; // 管线布局
        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 0; // 明确指定不使用push constants
        pipelineLayoutInfo.pPushConstantRanges = nullptr;

        VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout));

        // 图形管线
        VkGraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineInfo, nullptr, &graphicsPipeline));
    }

    void prepare() override {
        vulkanFrameWork::prepare();

        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffer();
        createDescriptorSetLayout();
        createDescriptorPool(); // 添加这行：在创建描述符集之前先创建描述符池
        createDescriptorSet();
        createGraphicsPipeline();
    }

    void render() override {
        updateUniformBuffer();
        vulkanFrameWork::prepareFrame();
        vulkanFrameWork::submitFrame();
    }
};

// 主函数
int main() {
    FrameWork::Locator::RegisterService<FrameWork::InputManager>
    (std::make_shared<FrameWork::InputManager>(FrameWork::VulkanWindow::GetInstance().GetWindow())
        );
    TriangleRenderer app;
    app.initVulkan();
    app.setWindow();
    app.prepare();
    app.renderLoop();
    return 0;
}
