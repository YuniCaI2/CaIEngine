//
// Created by 51092 on 25-5-6.
//

#include "FrameWork/vulkanFrameWork.h"
#include <array>

struct Vertex {
    float pos[3];
    float color[3];
};

struct UniformBufferObject {
    glm::mat4 projectionMatrix;
    glm::mat4 modelMatrix;
    glm::mat4 viewMatrix;
};

class TriangleApp : public vulkanFrameWork {
private:
    FrameWork::Buffer vertexBuffer;
    FrameWork::Buffer indexBuffer;
    FrameWork::Buffer uniformBuffer;
    
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet descriptorSet;
    VkPipeline graphicsPipeline;
    VkPipelineLayout pipelineLayout;
    
    std::vector<Vertex> vertexData = {
        {{0.0f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},  // 顶点 红色
        {{0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},   // 右上 绿色
        {{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}}   // 左上 蓝色
    };
    
    std::vector<uint16_t> indices = {0, 1, 2};

    struct {
        VkPipelineVertexInputStateCreateInfo inputState;
        std::vector<VkVertexInputBindingDescription> bindingDescriptions;
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
    } vertexInputState;

public:
    TriangleApp() {
        title = "Triangle Example";
        camera.type = Camera::CameraType::lookat;
        camera.setPosition(glm::vec3(0.0f, 0.0f, -2.0f));
        camera.setRotation(glm::vec3(0.0f, 0.0f, 0.0f));
        camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 256.0f);
    }    void setupVertexDescriptions() {
        // 顶点输入绑定描述
        vertexInputState.bindingDescriptions.resize(1);
        vertexInputState.bindingDescriptions[0].binding = 0;
        vertexInputState.bindingDescriptions[0].stride = sizeof(Vertex);
        vertexInputState.bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        // 顶点输入属性描述
        vertexInputState.attributeDescriptions.resize(2);
        
        // Position
        vertexInputState.attributeDescriptions[0].binding = 0;
        vertexInputState.attributeDescriptions[0].location = 0;
        vertexInputState.attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        vertexInputState.attributeDescriptions[0].offset = offsetof(Vertex, pos);
        
        // Color
        vertexInputState.attributeDescriptions[1].binding = 0;
        vertexInputState.attributeDescriptions[1].location = 1;
        vertexInputState.attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        vertexInputState.attributeDescriptions[1].offset = offsetof(Vertex, color);

        vertexInputState.inputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputState.inputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputState.bindingDescriptions.size());
        vertexInputState.inputState.pVertexBindingDescriptions = vertexInputState.bindingDescriptions.data();
        vertexInputState.inputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputState.attributeDescriptions.size());
        vertexInputState.inputState.pVertexAttributeDescriptions = vertexInputState.attributeDescriptions.data();
    }

    void createDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uboLayoutBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &uboLayoutBinding;

        VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout));
    }

    void createGraphicsPipeline() {
        // 加载着色器
        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};
        shaderStages[0] = loadShader("triangle/triangle.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
        shaderStages[1] = loadShader("triangle/triangle.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

        // 创建管线布局
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

        VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout));

        // 输入装配
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // 视口和裁剪
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)width;
        viewport.height = (float)height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = {width, height};

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        // 光栅化
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        // 多重采样
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // 深度测试
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        // 颜色混合
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        // 创建图形管线
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages.data();
        pipelineInfo.pVertexInputState = &vertexInputState.inputState;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;

        VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineInfo, nullptr, &graphicsPipeline));
    }    void createVertexBuffer() {
        VkDeviceSize bufferSize = sizeof(vertexData[0]) * vertexData.size();
        
        // 创建顶点缓冲区
        VK_CHECK_RESULT(vulkanDevice->createBuffer(
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &vertexBuffer,
            bufferSize,
            vertexData.data()
        ));
    }

    void createIndexBuffer() {
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
        
        // 创建索引缓冲区
        VK_CHECK_RESULT(vulkanDevice->createBuffer(
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &indexBuffer,
            bufferSize,
            indices.data()
        ));
    }

    void createUniformBuffers() {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);
        
        VK_CHECK_RESULT(vulkanDevice->createBuffer(
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &uniformBuffer,
            bufferSize
        ));
        
        // 设置描述符
        uniformBuffer.setupDescriptor();
    }

    void createDescriptorSets() {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &descriptorSetLayout;

        VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSet;
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &uniformBuffer.descriptor;

        vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
    }

    void updateUniformBuffer() {
        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        UniformBufferObject ubo{};
        ubo.modelMatrix = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.viewMatrix = camera.matrices.view;
        ubo.projectionMatrix = camera.matrices.perspective;

        uniformBuffer.copyTo(&ubo, sizeof(ubo));
    }

    virtual void prepare() override {
        vulkanFrameWork::prepare();
        
        setupVertexDescriptions();
        createDescriptorSetLayout();
        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffers();
        createDescriptorSets();
        createGraphicsPipeline();
        
        prepared = true;
    }

    virtual void buildCommandBuffers() override {
        VkCommandBufferBeginInfo cmdBufInfo{};
        cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VkClearValue clearValues[2];
        clearValues[0].color = {{0.0f, 0.0f, 0.2f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};

        VkRenderPassBeginInfo renderPassBeginInfo{};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass = renderPass;
        renderPassBeginInfo.renderArea.offset.x = 0;
        renderPassBeginInfo.renderArea.offset.y = 0;
        renderPassBeginInfo.renderArea.extent.width = width;
        renderPassBeginInfo.renderArea.extent.height = height;
        renderPassBeginInfo.clearValueCount = 2;
        renderPassBeginInfo.pClearValues = clearValues;

        for (int32_t i = 0; i < drawCmdBuffers.size(); ++i) {
            renderPassBeginInfo.framebuffer = frameBuffers[i];

            VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));

            vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

            VkViewport viewport{};
            viewport.width = (float)width;
            viewport.height = (float)height;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.extent.width = width;
            scissor.extent.height = height;
            scissor.offset.x = 0;
            scissor.offset.y = 0;
            vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

            vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

            VkBuffer vertexBuffers[] = {vertexBuffer.buffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(drawCmdBuffers[i], 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(drawCmdBuffers[i], indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);

            vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

            vkCmdDrawIndexed(drawCmdBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

            drawUI(drawCmdBuffers[i]);

            vkCmdEndRenderPass(drawCmdBuffers[i]);

            VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
        }
    }

    VkResult render() override {
        if (!prepared) return VK_NOT_READY;
        
        updateUniformBuffer();
        return VK_SUCCESS;
    }

    virtual void getEnabledFeatures() override {
        // 启用需要的设备特性
    }

    virtual void OnUpdateUIOverlay(FrameWork::VulkanUIOverlay* overlay) override {
        if (overlay->header("Settings")) {
            overlay->text("Triangle Example");
        }
    }

    ~TriangleApp() {
        if (device) {
            vkDestroyPipeline(device, graphicsPipeline, nullptr);
            vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
            vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
            
            vertexBuffer.destroy();
            indexBuffer.destroy();
            uniformBuffer.destroy();
        }    }
};

int main() {
    TriangleApp app;
    
    app.setWindow();
    app.initVulkan();
    app.prepare();
    app.renderLoop();
    
    return 0;
}