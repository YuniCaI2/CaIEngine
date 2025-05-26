//
// Created by 51092 on 25-5-10.
//

#ifndef VULKANDEVICE_H
#define VULKANDEVICE_H

#include<vector>
#include <string>

#include "VulkanTool.h"
#include "VulkanBuffer.h"

namespace FrameWork {
    class VulkanDevice {
    public:
        VkPhysicalDevice physicalDevice;
        VkInstance instance;
        VkDevice logicalDevice;
        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceFeatures features;
        // Features that have been enabled for use on the physical Device
        VkPhysicalDeviceFeatures enabledFeatures;
        VkPhysicalDeviceMemoryProperties memoryProperties;
        std::vector<VkQueueFamilyProperties> queueFamilyProperties;
        std::vector<std::string> supportedExtensions;
        VkCommandPool commandPool = VK_NULL_HANDLE;

        struct {
            uint32_t graphics;
            uint32_t compute;
            uint32_t transfer;
        } queueFamilyIndices;
        //重载类型转换
        operator VkDevice() const {
            return logicalDevice;
        }

        explicit VulkanDevice(VkPhysicalDevice physicalDevice, VkInstance instance);
        ~VulkanDevice();
        uint32_t getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32 * memTypeFound = nullptr) const;
        uint32_t getQueueFamilyIndex(VkQueueFlagBits flags) const;
        VkResult createLogicalDevice(VkPhysicalDeviceFeatures enabledFeatures, std::vector<const char*> enabledExtensions, void* pNextChain, bool useSwapChain = true, VkQueueFlags requestQueueType = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);
        VkResult createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, VkBuffer *buffer, VkDeviceMemory *memory, void *data);
        //支持封装后的Buffer
        VkResult createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, Buffer* buffer, VkDeviceSize size, void* data = nullptr);
        void copyBuffer(Buffer* src, Buffer* dst, VkQueue queue, VkBufferCopy* copyRegion = nullptr);
        VkCommandPool createCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT); // 允许单独重置命令池中的命令缓冲
        VkCommandBuffer createCommandBuffer(VkCommandBufferLevel level, VkCommandPool pool, bool begin = false);
        VkCommandBuffer createCommandBuffer(VkCommandBufferLevel level, bool begin = false);
        void flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, VkCommandPool pool, bool free = true);
        void flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free = true);
        bool extensionsSupported(std::string extension);
        VkFormat getSupportedDepthFormat(bool checkSamplingSupport);
    };
}



#endif //VULKANDEVICE_H
