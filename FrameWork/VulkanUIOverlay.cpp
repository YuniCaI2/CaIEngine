//
// Created by 51092 on 25-5-10.
//

#include "VulkanUIOverlay.h"

#include <iostream>

#include "VulkanTool.h"

static void check_vk_result(VkResult err) {
    if (err == 0)
        return;
    std::cerr << "ImGui Vulkan Error: VkResult = " << err << std::endl;
    // 不要立即 abort，而是输出错误信息
    if (err < 0) {
        std::cerr << "严重错误，但不终止程序执行" << std::endl;
    }
}



FrameWork::VulkanUIOverlay::VulkanUIOverlay() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiStyle &style = ImGui::GetStyle();
    style.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.0f, 0.0f, 0.0f, 0.1f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.8f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.8f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.1f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(1.0f, 1.0f, 1.0f, 0.2f);
    style.Colors[ImGuiCol_Button] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
    //控制全局的字体缩放
    ImGuiIO &io = ImGui::GetIO();
}


FrameWork::VulkanUIOverlay::~VulkanUIOverlay() {
    if (ImGui::GetCurrentContext()) {
        ImGui::DestroyContext();
    }
}




//为ImGui准备Vulkan资源
void FrameWork::VulkanUIOverlay::prepareResources(GLFWwindow* window,VkInstance instance,
    VkRenderPass &renderPass, VulkanSwapChain& swapchain, VkQueue queue) {

    // 基础验证
    if (!device || device->logicalDevice == VK_NULL_HANDLE) {
        std::cerr << "device nullptr！" << std::endl;
        return;
    }

    // 创建完整的描述符池 - ImGui 需要多种描述符类型
    VkDescriptorPoolSize poolSizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 1000;
    poolInfo.poolSizeCount = IM_ARRAYSIZE(poolSizes);
    poolInfo.pPoolSizes = poolSizes;

    VkResult result = vkCreateDescriptorPool(device->logicalDevice, &poolInfo, nullptr, &descriptorPool);
    if (result != VK_SUCCESS) {
        std::cerr << "ui descriptor error : " << result << std::endl;
        return;
    }

    std::cout << "ImGui..." << std::endl;

    // 初始化 GLFW 后端
    if (!ImGui_ImplGlfw_InitForVulkan(window, true)) {
        std::cerr << "ImGui GLFW error！" << std::endl;
        return;
    }

    // 创建初始化信息 - 添加缺失的字段
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = instance;
    init_info.PhysicalDevice = device->physicalDevice;
    init_info.Device = device->logicalDevice;
    init_info.QueueFamily = device->queueFamilyIndices.graphics;
    init_info.Queue = queue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = descriptorPool;
    init_info.RenderPass = renderPass;
    init_info.Subpass = 0;
    init_info.MinImageCount = static_cast<uint32_t>(swapchain.images.size());
    init_info.ImageCount = static_cast<uint32_t>(swapchain.images.size());
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = nullptr;
    init_info.CheckVkResultFn = check_vk_result;

    std::cout << "ImGui Vulkan..." << std::endl;

    ImGui_ImplVulkan_Init(&init_info);

    std::cout << "ImGui success！" << std::endl;
}


void FrameWork::VulkanUIOverlay::draw(const VkCommandBuffer &commandBuffer) {
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

void FrameWork::VulkanUIOverlay::resize(uint32_t width, uint32_t height) {
    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float) (width), (float) (height));
}

void FrameWork::VulkanUIOverlay::freeResources() {
    vkDestroyDescriptorPool(device->logicalDevice, descriptorPool, nullptr);
}

bool FrameWork::VulkanUIOverlay::header(const char *caption) {
    return ImGui::CollapsingHeader(caption, ImGuiTreeNodeFlags_DefaultOpen);
    //首次展示的时候展开
}

bool FrameWork::VulkanUIOverlay::checkBox(const char *caption, bool *value) {
    bool res = ImGui::Checkbox(caption, value);
    if (res) { updated = true; };
    return res;
}

bool FrameWork::VulkanUIOverlay::checkBox(const char *caption, int32_t *value) {
    bool val = (*value == 1);
    bool res = ImGui::Checkbox(caption, &val);
    *value = val;
    if (res) { updated = true; };
    return res;
}

bool FrameWork::VulkanUIOverlay::radioButton(const char *caption, bool value) {
    bool res = ImGui::RadioButton(caption, value);
    if (res) { updated = true; };
    return res;
}

bool FrameWork::VulkanUIOverlay::inputFloat(const char *caption, float *value, float step, uint32_t precision) {
    bool res = ImGui::InputFloat(caption, value, step, step * 10.0f);
    if (res) { updated = true; };
    return res;
}

bool FrameWork::VulkanUIOverlay::sliderFloat(const char *caption, float *value, float min, float max) {
    bool res = ImGui::SliderFloat(caption, value, min, max);
    if (res) { updated = true; };
    return res;
}

bool FrameWork::VulkanUIOverlay::sliderInt(const char *caption, int32_t *value, int32_t min, int32_t max) {
    bool res = ImGui::SliderInt(caption, value, min, max);
    if (res) { updated = true; };
    return res;
}

bool FrameWork::VulkanUIOverlay::comboBox(const char *caption, int32_t *itemindex, std::vector<std::string> items) {
    if (items.empty()) {
        return false;
    }
    std::vector<const char *> charitems;
    charitems.reserve(items.size());
    for (size_t i = 0; i < items.size(); i++) {
        charitems.push_back(items[i].c_str());
    }
    uint32_t itemCount = static_cast<uint32_t>(charitems.size());
    bool res = ImGui::Combo(caption, itemindex, &charitems[0], itemCount, itemCount);
    if (res) { updated = true; };
    return res;
}

bool FrameWork::VulkanUIOverlay::button(const char *caption) {
    bool res = ImGui::Button(caption);
    if (res) { updated = true; };
    return res;
}

bool FrameWork::VulkanUIOverlay::colorPicker(const char *caption, float *color) {
    bool res = ImGui::ColorEdit4(caption, color, ImGuiColorEditFlags_NoInputs);
    if (res) { updated = true; };
    return res;
}

void FrameWork::VulkanUIOverlay::text(const char *formatstr, ...) {
    va_list args;
    va_start(args, formatstr);
    ImGui::TextV(formatstr, args);
    va_end(args);
}
