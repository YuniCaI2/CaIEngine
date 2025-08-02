//
// Created by 51092 on 25-5-22.
//

#include "VulkanDebug.h"
#include<iostream>

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