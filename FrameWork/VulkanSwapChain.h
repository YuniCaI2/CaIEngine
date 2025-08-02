//
// Created by 51092 on 25-5-7.
//

#ifndef VULKANSWAPCHAIN_H
#define VULKANSWAPCHAIN_H

#include <vector>
#include "pubh.h"

#include "GLFW/glfw3.h"

class VulkanSwapChain {
private:
    VkInstance instance{VK_NULL_HANDLE};
    VkDevice device{VK_NULL_HANDLE};
    VkPhysicalDevice physicalDevice{VK_NULL_HANDLE};
    VkSurfaceKHR surface{VK_NULL_HANDLE};

public:
    VkFormat colorFormat{};
    VkColorSpaceKHR colorSpace{};
    VkSwapchainKHR swapChain{VK_NULL_HANDLE};
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    uint32_t queueNodeIndex{UINT32_MAX};
    uint32_t minImageCount{UINT32_MAX};

    void initSurface(GLFWwindow* window);
    void setContext(VkInstance instance, VkPhysicalDevice physicalDevice,VkDevice device);
    void create(uint32_t width, uint32_t height, bool vsync = false, bool fullScreen = false);
    VkResult acquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t& imageIndex);
    VkResult queuePresent(VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore = VK_NULL_HANDLE);
    void cleanup();
};



#endif //VULKANSWAPCHAIN_H
