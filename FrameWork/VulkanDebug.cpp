//
// Created by 51092 on 25-5-22.
//

#include "VulkanDebug.h"
#include<iostream>
#include<iterator>
#include<array>

#include "vulkanFrameWork.h"

// 定义静态成员变量
VkDebugUtilsMessengerEXT FrameWork::VulkanDebug::debugUtilsMessenger = VK_NULL_HANDLE;

// 函数指针定义
PFN_vkCreateDebugUtilsMessengerEXT FrameWork::VulkanDebug::vkCreateDebugUtilsMessengerEXT = nullptr;
PFN_vkDestroyDebugUtilsMessengerEXT FrameWork::VulkanDebug::vkDestroyDebugUtilsMessengerEXT = nullptr;
PFN_vkCmdBeginDebugUtilsLabelEXT FrameWork::debugUtils::vkCmdBeginDebugUtilsLabelEXT = nullptr;
PFN_vkCmdEndDebugUtilsLabelEXT FrameWork::debugUtils::vkCmdEndDebugUtilsLabelEXT = nullptr;

VKAPI_ATTR VkBool32  VKAPI_CALL FrameWork::VulkanDebug::debugUtilsMessageCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData) {
    //大于警告都需要发送
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    }
    return VK_FALSE;
}

void FrameWork::VulkanDebug::setDebugging(VkInstance instance) {
    // 获取扩展函数指针
    vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

    // 验证扩展函数可用性
    if (vkCreateDebugUtilsMessengerEXT == nullptr) {
        throw std::runtime_error("Debug utils extension not available");
    }

    // 创建调试信使
    VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCI{};
    setupDebuggingMessengerCreateInfo(debugUtilsMessengerCI);

    VkResult result = vkCreateDebugUtilsMessengerEXT(instance, &debugUtilsMessengerCI, nullptr, &debugUtilsMessenger);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create debug messenger");
    }
}

void FrameWork::VulkanDebug::freeDebugCallBack(VkInstance instance) {
    if (debugUtilsMessenger != VK_NULL_HANDLE && vkDestroyDebugUtilsMessengerEXT != nullptr) {
        vkDestroyDebugUtilsMessengerEXT(instance, debugUtilsMessenger, nullptr);
        debugUtilsMessenger = VK_NULL_HANDLE;
    }
}

void FrameWork::VulkanDebug::setupDebuggingMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &debug) {
    debug.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    //接受消息的类型是，一般类型的消息和验证层消息
    debug.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debug.pfnUserCallback = debugUtilsMessageCallback;
}

void FrameWork::AABBDeBugging::Init(const std::string &shaderName,uint32_t colorAttachment, uint32_t depthAttachmentID) {
    //RenderPass Init

    std::array<VkAttachmentDescription, 2> attachments = {};
    // Color attachment
    attachments[0].format = vulkanRenderAPI.GetVulkanSwapChain().colorFormat;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    // Depth attachment
    attachments[1].format = vulkanRenderAPI.GetDepthFormat();
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
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
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dependencyFlags = 0;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpassDescription;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    VK_CHECK_RESULT(vkCreateRenderPass(vulkanRenderAPI.vulkanDevice->logicalDevice, &renderPassInfo, nullptr, &debugRenderPass));
    vulkanRenderAPI.RegisterRenderPass(debugRenderPass, "debugRenderPass");

    //framebuffer
    vulkanRenderAPI.CreateFrameBuffer(frameBufferID,
        {colorAttachment, depthAttachmentID},
        vulkanRenderAPI.GetFrameWidth(), vulkanRenderAPI.GetFrameHeight(), debugRenderPass);
    frameBuffers = vulkanRenderAPI.getByIndex<VulkanFBO>(frameBufferID)->framebuffers;

    //Pipeline
    auto bindingDescription = LineVertex::getBindingDescription();
    auto attributeDescription = LineVertex::getAttributeDescription();
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = vulkanRenderAPI.GetFrameWidth();
    viewport.height = vulkanRenderAPI.GetFrameHeight();
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor = {};
    scissor.extent.width = vulkanRenderAPI.GetFrameWidth();
    scissor.extent.height = vulkanRenderAPI.GetFrameHeight();
    scissor.offset.x = 0.0f;
    scissor.offset.y = 0.0f;
    uint32_t pipelineInfoId = -1;
    vulkanRenderAPI.InitPipelineInfo(pipelineInfoId);
    vulkanRenderAPI.LoadPipelineShader(pipelineInfoId, shaderName, VK_SHADER_STAGE_VERTEX_BIT);
    vulkanRenderAPI.LoadPipelineShader(pipelineInfoId, shaderName, VK_SHADER_STAGE_FRAGMENT_BIT);
    vulkanRenderAPI.AddPipelineVertexAttributeDescription(pipelineInfoId, attributeDescription);
    vulkanRenderAPI.AddPipelineVertexBindingDescription(pipelineInfoId, bindingDescription);
    vulkanRenderAPI.SetPipelineViewPort(pipelineInfoId, viewport);
    vulkanRenderAPI.SetPipelineScissor(pipelineInfoId, scissor);
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    vulkanRenderAPI.SetPipelineRasterizationState(pipelineInfoId, rasterizer);
    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    vulkanRenderAPI.SetPipelineDepthStencilState(pipelineInfoId, depthStencil);
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    vulkanRenderAPI.SetPipelineMultiSampleState(pipelineInfoId, multisampling);
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };
    uniformDescriptorSetLayout = vulkanRenderAPI.CreateDescriptorSetLayout(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,  VK_SHADER_STAGE_VERTEX_BIT);
    vulkanRenderAPI.SetPipelineInputAssembly(pipelineInfoId, inputAssembly);
    vulkanRenderAPI.AddPipelineColorBlendState(pipelineInfoId, true, BlendOp::Opaque);
    vulkanRenderAPI.CreateVulkanPipeline(debugPipelineID, "DeBugPipeline", pipelineInfoId, "debugRenderPass", 0,
        {uniformDescriptorSetLayout}, 1, 0
        );
    debugPipeline = vulkanRenderAPI.getByIndex<FrameWork::VulkanPipeline>(pipelineInfoId)->pipeline;
    debugPipelineLayout = vulkanRenderAPI.getByIndex<VulkanPipeline>(pipelineInfoId)->pipelineLayout;
}

void FrameWork::AABBDeBugging::GenerateAABB(uint32_t modelID) {
        auto model = vulkanRenderAPI.getByIndex<Model>(modelID);
        if (model->inUse == false) {
            return;
        }
        FrameWork::Buffer vertexBuffer{VK_NULL_HANDLE};
        FrameWork::Buffer indexBuffer{VK_NULL_HANDLE};
        auto modelIndices = GenerateLineIndices(lines.size());
        auto modelAABB = GenerateLineVertex(model->aabb, glm::vec3(255.0f, 0.0f, 0.0f));
        lines.insert(lines.end(), std::make_move_iterator( modelAABB.begin()),
            std::make_move_iterator( modelAABB.end()));
        indices.insert(indices.end(), std::make_move_iterator(modelIndices.begin()),
            std::make_move_iterator( modelIndices.end()));
        for (auto& aabb : *model->triangleBoundingBoxs) {
            auto triIndices = GenerateLineIndices(lines.size());
            auto triAABB = GenerateLineVertex(aabb, glm::vec3(0.0f, 255.0f, 0.0f));
            lines.insert(lines.end(), std::make_move_iterator( triAABB.begin()),
                std::make_move_iterator( triAABB.end()));
            indices.insert(indices.end(), std::make_move_iterator(triIndices.begin()),
                std::make_move_iterator( triIndices.end()));
        }
        VkDeviceSize vertexBufferSize = sizeof(LineVertex) * lines.size();
        vulkanRenderAPI.CreateGPUBuffer(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexBuffer, lines.data());

        VkDeviceSize indicesBufferSize = sizeof(uint32_t) * indices.size();
        vulkanRenderAPI.CreateGPUBuffer(indicesBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indexBuffer, indices.data());
        vertexBuffers.emplace(modelID, vertexBuffer);
        indexBuffers.emplace(modelID, indexBuffer);
        indicesCounts.emplace(modelID, indices.size());
        lines.clear();
        indices.clear();

        MaterialCreateInfo materialInfo{};
        materialInfo.UniformDescriptorLayouts = {uniformDescriptorSetLayout};
        materialInfo.UniformData = {std::make_pair(&ubo, sizeof(UniformBufferObject))};
        uint32_t materialID = -1;
        vulkanRenderAPI.CreateMaterial(materialID, materialInfo);
        materialIds.emplace(modelID, materialID);
}

void FrameWork::AABBDeBugging::Draw(VkCommandBuffer cmdBuffer) {
    VkClearValue clearValues[2];
    clearValues[0].color = vulkanRenderAPI.defaultClearColor;
    clearValues[1].depthStencil = {1.0f, 0};
    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.renderPass = debugRenderPass;
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.framebuffer = frameBuffers[vulkanRenderAPI.GetCurrentFrame()];
    renderPassInfo.renderArea.offset.x = 0;
    renderPassInfo.renderArea.offset.y = 0;
    renderPassInfo.renderArea.extent.width = vulkanRenderAPI.GetFrameWidth();
    renderPassInfo.renderArea.extent.height = vulkanRenderAPI.GetFrameHeight();
    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    VkViewport viewport = {};
    viewport.width = (float) vulkanRenderAPI.windowWidth;
    viewport.height = (float) vulkanRenderAPI.windowHeight;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(vulkanRenderAPI.GetCurrentCommandBuffer(), 0, 1, &viewport);
    VkRect2D scissor = {};
    scissor.extent.width = vulkanRenderAPI.windowWidth;
    scissor.extent.height = vulkanRenderAPI.windowHeight;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vkCmdSetScissor(vulkanRenderAPI.GetCurrentCommandBuffer(), 0, 1, &scissor);
    vkCmdBindPipeline(vulkanRenderAPI.GetCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, debugPipeline);
    VkDeviceSize offset = {0};
    for (auto& [modelID, vertexBuffer] : vertexBuffers) {
        if (vulkanRenderAPI.getByIndex<Model>(modelID)->inUse == false) {
            //这样没有删除可以保证线程安全但是占用内存
            continue;
        }
        auto material = vulkanRenderAPI.getByIndex<Material>(materialIds[modelID]);
        std::vector descriptorOffset = {
            vulkanRenderAPI.currentFrame * material->uniformBufferSizes[0]
            / vulkanRenderAPI.MaxFrame
        };
        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, debugPipelineLayout, 0,
            1,
            &material->uniformDescriptorSets[0], 1, descriptorOffset.data());
        vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer.buffer, &offset);
        vkCmdBindIndexBuffer(cmdBuffer, indexBuffers[modelID].buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmdBuffer, indicesCounts[modelID], 1, 0, 0, 0);
    }
    vkCmdEndRenderPass(vulkanRenderAPI.GetCurrentCommandBuffer());
}

void FrameWork::AABBDeBugging::Update(const glm::mat4 &viewMatrix,
    const glm::mat4 &projectionMatrix) {
    ubo.view = viewMatrix;
    ubo.proj = projectionMatrix;

    for (auto& m : materialIds) {
        ubo.model = glm::mat4(1.0f);
        auto model = vulkanRenderAPI.getByIndex<Model>(m.first);
        if (model->inUse == false) {
            //TODO： 保证多飞行帧线程安全
            //因为无法保证安全现在只是不更新和渲染
            continue;
        }
        ubo.model = glm::translate(ubo.model, model->position);
        auto material = vulkanRenderAPI.getByIndex<Material>(m.second);
        vulkanRenderAPI.UpdateUniformBuffer({material->uniformBuffer[0]},{&ubo}, {sizeof(decltype(ubo))},
                                                        vulkanRenderAPI.currentFrame * material->uniformBufferSizes[0] /
                                                        vulkanRenderAPI.MaxFrame);
    }

}

void FrameWork::AABBDeBugging::Destroy() {
    for (auto& v : vertexBuffers) {
        v.second.destroy();
    }
    for (auto& i : indexBuffers) {
        i.second.destroy();
    }
}

std::vector<FrameWork::AABBDeBugging::LineVertex> FrameWork::AABBDeBugging::GenerateLineVertex(const AABB &aabb, glm::vec3&& color) {
    std::vector<LineVertex> lineVertex(8);

    lineVertex[0].position = {aabb.min.x, aabb.min.y, aabb.min.z};
    lineVertex[1].position = {aabb.max.x, aabb.min.y, aabb.min.z};
    lineVertex[2].position = {aabb.min.x, aabb.max.y, aabb.min.z};
    lineVertex[3].position = {aabb.min.x, aabb.min.y, aabb.max.z};
    lineVertex[4].position = {aabb.max.x, aabb.max.y, aabb.min.z};
    lineVertex[5].position = {aabb.min.x, aabb.max.y, aabb.max.z};
    lineVertex[6].position = {aabb.max.x, aabb.min.y, aabb.max.z};
    lineVertex[7].position = {aabb.max.x, aabb.max.y, aabb.max.z};

    for (int i = 0; i < 8; i++) {
        lineVertex[i].color = color;
    }

    return lineVertex;
}

std::vector<uint32_t> FrameWork::AABBDeBugging::GenerateLineIndices(uint32_t baseID) {
    std::vector<uint32_t> indices = {
        0, 1, 0, 2, 1, 4, 2, 4,
        3, 5, 5, 7, 7, 6, 6, 3,
        2, 5, 4, 7, 1, 6, 0, 3
    };

    for (auto& i : indices) {
        i = baseID + i;
    }
    return indices;
}

void FrameWork::debugUtils::setup(VkInstance instance) {
    // 获取调试标签相关的函数指针
    vkCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT) vkGetInstanceProcAddr(instance, "vkCmdBeginDebugUtilsLabelEXT");
    vkCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT) vkGetInstanceProcAddr(instance, "vkCmdEndDebugUtilsLabelEXT");
}

//这里的是你为了展示性能,上传的gpu的内容进行测试
void FrameWork::debugUtils::cmdBeginLabel(VkCommandBuffer cmdBuffer, std::string caption, glm::vec4 color) {
    if (vkCmdBeginDebugUtilsLabelEXT != nullptr) {
        VkDebugUtilsLabelEXT label{};
        label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
        label.pLabelName = caption.c_str();
        memcpy(label.color, &color, sizeof(float) * 4);
        vkCmdBeginDebugUtilsLabelEXT(cmdBuffer, &label);
    }
}

void FrameWork::debugUtils::cmdEndLabel(VkCommandBuffer cmdBuffer) {
    if (vkCmdEndDebugUtilsLabelEXT != nullptr) {
        vkCmdEndDebugUtilsLabelEXT(cmdBuffer);
    }
}