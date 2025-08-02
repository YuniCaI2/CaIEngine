//
// Created by 51092 on 25-5-22.
//

#ifndef VULKANDEBUG_H
#define VULKANDEBUG_H

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <string>

namespace FrameWork {
    class VulkanDebug {
    public:
        static VkDebugUtilsMessengerEXT debugUtilsMessenger;

        // 函数指针声明
        static PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
        static PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;

        static VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessageCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
            void *pUserData);

        static void setDebugging(VkInstance instance);
        static void freeDebugCallBack(VkInstance instance);
        static void setupDebuggingMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &debug);
    };

    namespace debugUtils {
        // 函数指针声明
        extern PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT;
        extern PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT;

        void setup(VkInstance instance);
        void cmdBeginLabel(VkCommandBuffer cmdBuffer, std::string caption, glm::vec4 color);
        void cmdEndLabel(VkCommandBuffer cmdBuffer);
    }
}

#endif //VULKANDEBUG_H