//
// Created by 51092 on 25-5-22.
//

#ifndef VULKANDEBUG_H
#define VULKANDEBUG_H
#include <glm/glm.hpp>
#include <string>
#include <chrono>
#define VK_NO_PROTOTYPES
#include <volk.h>

namespace FrameWork {
    class VulkanDebug {
    public:
        static VkDebugUtilsMessengerEXT debugUtilsMessenger;
        //Default debug callback
        static VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessageCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
            void *pUserData);

        static void setDebugging(VkInstance instance);

        static void freeDebugCallBack(VkInstance instance);

        static void setupDebuggingMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debug);
    };


    class debugUtils {
    public:
        static void setup(VkInstance instance);
        static void cmdBeginLabel(VkCommandBuffer cmdBuffer, std::string caption, glm::vec4 color);
        static void cmdEndLabel(VkCommandBuffer cmdBuffer);
    };
}



#endif //VULKANDEBUG_H
