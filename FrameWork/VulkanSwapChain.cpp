//
// Created by 51092 on 25-5-7.
//

#include "VulkanSwapChain.h"

#include <assert.h>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include "VulkanTool.h"

//获取表面的队列族索引、表面的Format 和ColorSpace
void VulkanSwapChain::initSurface(GLFWwindow *window) {
    VK_CHECK_RESULT(glfwCreateWindowSurface(instance, window, nullptr, &surface));
    //使用glfw的window实现表面的构建
    //得到可用的队列家族
    uint32_t queueCount;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, nullptr);
    assert(queueCount > 0);
    std::vector<VkQueueFamilyProperties> queueProps(queueCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, queueProps.data());
    /*迭代每个队列以了解它是否支持表示：
    查找具有当前支持的队列
    将用于向窗口系统呈现交换链图像*/
    std::vector<VkBool32> supportPresent(queueCount);
    for (uint32_t i = 0; i < queueCount; i++) {
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &supportPresent[i]);
        //物理设备、队列索引、呈现的表面实例、输出是否支持
    }
    uint32_t graphicsQueueNodeIndex = UINT32_MAX;
    uint32_t presentQueueNodeIndex = UINT32_MAX;
    for (uint32_t i = 0; i < queueCount; i++) {
        if ((queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
            if (graphicsQueueNodeIndex == UINT32_MAX) {
                graphicsQueueNodeIndex = i;
            }
            if (supportPresent[i] == VK_TRUE) {
                graphicsQueueNodeIndex = i;
                presentQueueNodeIndex = i;
                break;
            }
        }
    }
    if (presentQueueNodeIndex == UINT32_MAX) {
        //这里是为了防止没有队列家族同时匹配图像呈现和图像渲染
        for (uint32_t j = 0; j < queueCount; j++) {
            if (supportPresent[j] == VK_TRUE) {
                presentQueueNodeIndex = j;
                break;
            }
        }
    }
    if (graphicsQueueNodeIndex == UINT32_MAX || presentQueueNodeIndex == UINT32_MAX) {
        std::cerr << "Failed to find queue node!" << std::endl;
        exit(-1);
    }

    if (graphicsQueueNodeIndex != presentQueueNodeIndex) {
        std::cerr << "现在不支持将图像渲染和呈现队列分离" << std::endl;
    }

    queueNodeIndex = graphicsQueueNodeIndex;

    //这里是得到支持的表面格式是
    uint32_t formatCount;
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr) != VK_SUCCESS) {
        std::cerr << "Failed to get surface formats!" << std::endl;
        exit(-1);
    }
    assert(formatCount > 0);

    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data()) != VK_SUCCESS) {
        std::cerr << "Failed to get surface formats!" << std::endl;
        exit(-1);
    }
    //我们想要获得最适合要求的格式，因此我们尝试从一组首选格式中获得一种，
    //将格式初始化为实现返回的第一个格式，以防我们找不到首选格式格式之一
    VkSurfaceFormatKHR selectedFormat = formats[0];
    std::vector<VkFormat> preferredImageFormats = {
        VK_FORMAT_B8G8R8A8_SRGB,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_A8B8G8R8_UNORM_PACK32
    };
    for (auto &availableFormat: formats) {
        if (std::find(preferredImageFormats.begin(), preferredImageFormats.end(), availableFormat.format) !=
            preferredImageFormats.end()) {
            selectedFormat = availableFormat;
            break;
        }
    }
    colorSpace = selectedFormat.colorSpace;
    colorFormat = selectedFormat.format;
}

void VulkanSwapChain::setContext(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device) {
    this->instance = instance;
    this->physicalDevice = physicalDevice;
    this->device = device;
}

void VulkanSwapChain::create(uint32_t width, uint32_t height, bool vsync, bool fullScreen) {
    assert(physicalDevice);
    assert(device);
    assert(instance);


    //Store the current swap chain handle so we can use it later on to ease up recreation
    //简化重新创建
    VkSwapchainKHR oldSwapChain = swapChain;

    //Get physical device surface properties and formats
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);
    //这里的surface的大小是根据glfw创建的window大小得到的
    //也使用glfwGetFrameBufferSize()可以得到相应大小

    VkExtent2D swapChainExtent = {};
    //如果width和height 的值等于special值 0xFFFFFFFF,则surface 的大小将由 swapChain设置
    if (surfaceCapabilities.currentExtent.width == (uint32_t)-1) {
        //未定义则大小为请求大小
        swapChainExtent.width = width;
        swapChainExtent.height = height;
    }else {
        //定义了则为交换链的大小
        swapChainExtent = surfaceCapabilities.currentExtent;
        width = surfaceCapabilities.currentExtent.width;
        height = surfaceCapabilities.currentExtent.height;
    }

    //为交换链选择呈现模式
    uint32_t presentModeCount;
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr) != VK_SUCCESS) {
        std::cerr << "Failed to get surface present modes!" << std::endl;
        exit(-1);
    }
    assert(presentModeCount > 0);
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data()) != VK_SUCCESS) {
        std::cerr << "Failed to get surface present modes!" << std::endl;
        exit(-1);
    }

    VkPresentModeKHR swapChainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    if (! vsync) {
        for (size_t i = 0; i < presentModes.size(); i++) {
            if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
                swapChainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
            else if (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                swapChainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            }
        }
    }

    // 确定图片的数量
    uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
    if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount) {
        imageCount = surfaceCapabilities.maxImageCount;
    }

    //找到表面的变换
    VkSurfaceTransformFlagBitsKHR preTransform;
    if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        //我们更希望没有人任何旋转变化
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    else {
        preTransform = surfaceCapabilities.currentTransform;
    }

    //找到合适的alpha混和的格式
    //为了设置窗口和桌面的其他元素之间是如何进行合成的
    VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR, // 预先乘上alpha通道
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,// 后乘alpha通道
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR
    };
    for (auto &alphaFlag: compositeAlphaFlags) {
        if (surfaceCapabilities.supportedCompositeAlpha & alphaFlag) {
            compositeAlpha = alphaFlag;
            break;
        }
    }

    VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
    swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCreateInfo.surface = surface;
    swapChainCreateInfo.minImageCount = imageCount;
    swapChainCreateInfo.imageFormat = colorFormat;
    swapChainCreateInfo.imageColorSpace = colorSpace;
    swapChainCreateInfo.imageExtent = {swapChainExtent.width, swapChainExtent.height};
    swapChainCreateInfo.imageArrayLayers = 1;
    swapChainCreateInfo.queueFamilyIndexCount = 0;
    swapChainCreateInfo.presentMode = swapChainPresentMode;
    swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapChainCreateInfo.preTransform = preTransform;
    swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // 表示共享
    swapChainCreateInfo.oldSwapchain = oldSwapChain;
    swapChainCreateInfo.compositeAlpha = compositeAlpha;
    //Vulkan 可以丢弃被系统UI遮挡的部分（有点意思）
    swapChainCreateInfo.clipped = VK_TRUE;

    //能够作为传输源，如果支持的话
    if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
        swapChainCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }

    //传输的目标
    if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
        swapChainCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }

    VK_CHECK_RESULT(vkCreateSwapchainKHR(device, &swapChainCreateInfo, nullptr, &swapChain));

    //如果交换链被重新创建，销毁旧的交换链以及资源
    if (oldSwapChain != VK_NULL_HANDLE) {
        for (size_t i = 0; i < images.size(); i++) {
            if (imageViews[i] != VK_NULL_HANDLE) {
                vkDestroyImageView(device, imageViews[i], nullptr);
                imageViews[i] = VK_NULL_HANDLE;
            }
        }
        vkDestroySwapchainKHR(device, oldSwapChain, nullptr);
    }

    //这里和上面的值不太一样
    imageCount = 0;
    VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr));
    images.resize(imageCount);
    VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device, swapChain, &imageCount, images.data()));
    imageViews.resize(images.size());
    for (size_t i = 0; i < images.size(); i++) {
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = images[i];
        viewInfo.pNext = nullptr;
        viewInfo.format = colorFormat;
        viewInfo.components = {
            VK_COMPONENT_SWIZZLE_R, // RGBA四个通道映射
            VK_COMPONENT_SWIZZLE_G,
            VK_COMPONENT_SWIZZLE_B,
            VK_COMPONENT_SWIZZLE_A
        };
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.flags = 0;
        VK_CHECK_RESULT(vkCreateImageView(device, &viewInfo, nullptr, &imageViews[i]));
    }
}

VkResult VulkanSwapChain::acquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t& imageIndex) {
    // By setting timeout to UINT64_MAX we will always wait until the next image has been acquired or an actual error is thrown
    // With that we don't have to handle VK_NOT_READY
    //这里就只是做了层浅包装
    return vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, presentCompleteSemaphore, (VkFence)nullptr, &imageIndex);
}

VkResult VulkanSwapChain::queuePresent(VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore) {
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapChain;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;
    presentInfo.pNext = nullptr;
    //先判断有没有需要等待的信号量
    if (waitSemaphore != VK_NULL_HANDLE) {
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &waitSemaphore;
    }
    return vkQueuePresentKHR(queue, &presentInfo);
}

void VulkanSwapChain::cleanup() {
    if (swapChain != VK_NULL_HANDLE) {
        for (size_t i = 0; i < imageViews.size(); i++) {
            if (imageViews[i] != VK_NULL_HANDLE) {
                vkDestroyImageView(device, imageViews[i], nullptr);
                imageViews[i] = VK_NULL_HANDLE;
            }
        }
        //Image的生命周期所有权在swapchain之中
        vkDestroySwapchainKHR(device, swapChain, nullptr);
        swapChain = VK_NULL_HANDLE;
    }
    if (surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance, surface, nullptr);
        surface = VK_NULL_HANDLE;
    }
    surface = VK_NULL_HANDLE;
    swapChain = VK_NULL_HANDLE;
}
