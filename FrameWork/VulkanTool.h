//
// Created by 51092 on 25-5-8.
//

#ifndef VULKANTOOL_H
#define VULKANTOOL_H

// Custom define for better code readability
#define VK_FLAGS_NONE 0
// Default fence timeout in nanoseconds
#define DEFAULT_FENCE_TIMEOUT 100000000000

#include <intrin.h>

#include "pubh.h"
#include<functional>

#define VK_CHECK_RESULT(f)                                                      \
{                                                                               \
VkResult res = (f);                                                         \
if (res != VK_SUCCESS)                                                      \
{                                                                           \
std::cerr << "Fatal : VkResult is \"" << res << "\" in " << __FILE__    \
<< " at line " << __LINE__ << "\n";                           \
exit(-1);                                              \
}                                                                           \
}

namespace VulkanTool {

    //因为GLFW的特殊性质，这里要对其进行包装
    //回调包装
    using KeyCallback = std::function<void(int, int, int, int)>;
    using MouseButtonCallback = std::function<void(int, int, int)>;
    using CursorPosCallback = std::function<void(double, double)>;
    using ScrollCallback = std::function<void(double, double)>;

    //手动硬编码
    inline std::string resourcePath = "../resources/";

    inline std::string getAssetPath() {
        if (resourcePath.empty()) {
            return "";
        }

        // 确保路径末尾有斜杠
        if (resourcePath.back() != '/' && resourcePath.back() != '\\') {
            return resourcePath + '/';
        }

        return resourcePath;
    }

    inline std::string getShaderBasePath() {
        if (! resourcePath.empty()) {
            return resourcePath + "shaders/";
        }
        else {
            std::cerr << "Error: No resource path set!" << "\n";
            return "";
        }
    }

    inline VkBool32 getSupportedDepthStencilFormat(VkPhysicalDevice physicalDevice, VkFormat* depthFormat) {
        std::vector<VkFormat> formatList = {
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT,
            VK_FORMAT_D16_UNORM_S8_UINT
        };

        for (auto& format : formatList) {
            VkFormatProperties formatProperties;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);
            //检查优化平铺模式下的功能是否支持深度附件模板
            if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                *depthFormat = format;
                return VK_TRUE;
            }
        }
        return VK_FALSE;
    }

    inline VkBool32 getSupportDepthFormat(VkPhysicalDevice physicalDevice, VkFormat* depthFormat) {
        //模板是可选的
        std::vector<VkFormat> formatList = {
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D24_UNORM_S8_UINT,
            VK_FORMAT_D16_UNORM_S8_UINT,
            VK_FORMAT_D16_UNORM
        };

        for (auto& format : formatList) {
            VkFormatProperties formatProps;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);
            if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                *depthFormat = format;
                return VK_TRUE;
            }
        }

        return VK_FALSE;
    }

    inline VkShaderModule loadShader(const std::string& fileName, VkDevice device) {
        std::ifstream is(fileName, std::ios::ate | std::ios::binary | std::ios::in);

        //打开时光标到文件末尾，以二进制模式打开，以读取模式打开
        if (is.is_open()) {
            size_t size = is.tellg();
            is.seekg(0, std::ios::beg);
            char* shaderCode = new char[size];
            is.read(shaderCode, size);
            is.close();

            assert(size > 0);

            VkShaderModule shaderModule;
            VkShaderModuleCreateInfo moduleCreateInfo{};
            moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            moduleCreateInfo.codeSize = size;
            moduleCreateInfo.pCode = (uint32_t*)shaderCode;

            VK_CHECK_RESULT(vkCreateShaderModule(device, &moduleCreateInfo, NULL, &shaderModule));

            delete[] shaderCode;

            return shaderModule;
        }else {
            std::cerr << "Error: Could not open shader file \"" << fileName << "\"" << "\n";
            return VK_NULL_HANDLE;
        }
    }

    inline void setImageLayout(
        VkCommandBuffer cmdBuffer,
        VkImage image,
        VkImageLayout oldImageLayout,
        VkImageLayout newImageLayout,
        VkImageSubresourceRange subresourceRange,
        VkPipelineStageFlags srcStageMask,
        VkPipelineStageFlags dstStageMask
        ) {
        //创建Image Barrier
        VkImageMemoryBarrier imageMemoryBarrier = {};
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.oldLayout = oldImageLayout;
        imageMemoryBarrier.newLayout = newImageLayout;
        imageMemoryBarrier.image = image;
        imageMemoryBarrier.subresourceRange = subresourceRange;

        // Old Source layout
        // Source access Mask 管理必须完成的操作在老的布局上
        switch (oldImageLayout) {
            case VK_IMAGE_LAYOUT_UNDEFINED:
                // 图片的布局没有被定义
                // 仅仅作为初始状态有效
                //不需要任何标志，仅为完整性列出
                imageMemoryBarrier.srcAccessMask = 0;
                break;

            case VK_IMAGE_LAYOUT_PREINITIALIZED:
                //图像已经预初始化
                //仅作为线性图像的初始布局有效
                //保留内存内容
                //确保写入完成
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                break;

            default:
                break;
        }

        // new Target Layout
        switch (newImageLayout) {
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                break;

            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                //做了个保险
                if (imageMemoryBarrier.srcAccessMask == 0) {
                    imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
                }
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                break;

            default:
                break;
        }

        vkCmdPipelineBarrier(
            cmdBuffer,
            srcStageMask,
            dstStageMask,
            0,
            0, nullptr,
            0, nullptr,
            1, &imageMemoryBarrier
            );
    }

    
    //单层资源
    inline void transitionImageLayout(
        VkCommandBuffer cmdBuffer,
        VkImage image,
        VkImageAspectFlags aspectMask,
        VkImageLayout oldImageLayout,
        VkImageLayout newImageLayout,
        VkPipelineStageFlags srcStageMask,
        VkPipelineStageFlags dstStageMask
        ) {
        //子资源规范了图片的具体哪些部分会被转换
        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask = aspectMask;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = 1;
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = 1;

        //在不声明的情况下，必须将使用的函数写在前面
        setImageLayout(
            cmdBuffer,
            image,
            oldImageLayout,
            newImageLayout,
            subresourceRange,
            srcStageMask,
            dstStageMask
            );
    }

    inline VkCommandBuffer beginSingleTimeCommands(const VkDevice &device, const VkCommandPool &commandPool) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        //命令缓冲区被使用一次后被丢弃

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    inline void endSingleTimeCommands(const VkDevice &device, VkQueue queue, const VkCommandPool &commandPool,
                                  VkCommandBuffer commandBuffer) {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue);
        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }


}

#endif //VULKANTOOL_H