//
// Created by 51092 on 25-5-8.
//

#ifndef VULKANTOOL_H
#define VULKANTOOL_H

// Custom define for better code readability
#define VK_FLAGS_NONE 0
// Default fence timeout in nanoseconds
#define DEFAULT_FENCE_TIMEOUT 100000000000


#include "pubh.h"
#include<functional>
#include <queue>
#include "Logger.h"
#include "VulkanDevice.h"

#define VK_CHECK_RESULT(f)                                                      \
{                                                                               \
VkResult res = (f);                                                         \
if (res != VK_SUCCESS)                                                      \
{                                                                           \
ERROR("vulkan has error ! the res is : {}", static_cast<int>(res)); \
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
        if (!resourcePath.empty()) {
            return resourcePath + "shaders/";
        } else {
            std::cerr << "Error: No resource path set!" << "\n";
            return "";
        }
    }

    inline VkBool32 getSupportedDepthStencilFormat(VkPhysicalDevice physicalDevice, VkFormat *depthFormat) {
        std::vector<VkFormat> formatList = {
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT,
            VK_FORMAT_D16_UNORM_S8_UINT
        };

        for (auto &format: formatList) {
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

    inline VkBool32 getSupportDepthFormat(VkPhysicalDevice physicalDevice, VkFormat *depthFormat) {
        //模板是可选的
        std::vector<VkFormat> formatList = {
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D24_UNORM_S8_UINT,
            VK_FORMAT_D16_UNORM_S8_UINT,
            VK_FORMAT_D16_UNORM
        };

        for (auto &format: formatList) {
            VkFormatProperties formatProps;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);
            if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                *depthFormat = format;
                return VK_TRUE;
            }
        }

        return VK_FALSE;
    }

    inline VkShaderModule loadShader(const std::string &fileName, VkDevice device) {
        std::ifstream is(fileName, std::ios::ate | std::ios::binary | std::ios::in);
        is.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        //打开时光标到文件末尾，以二进制模式打开，以读取模式打开
        if (is.is_open()) {
            size_t size = is.tellg();
            is.seekg(0, std::ios::beg);
            char *shaderCode = new char[size];
            is.read(shaderCode, size);
            is.close();

            assert(size > 0);

            VkShaderModule shaderModule;
            VkShaderModuleCreateInfo moduleCreateInfo{};
            moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            moduleCreateInfo.codeSize = size;
            moduleCreateInfo.pCode = (uint32_t *) shaderCode;

            VK_CHECK_RESULT(vkCreateShaderModule(device, &moduleCreateInfo, nullptr, &shaderModule));

            delete[] shaderCode;

            return shaderModule;
        } else {
            std::cerr << "Error: Could not open shader file \"" << fileName << "\"" << "\n";
            return VK_NULL_HANDLE;
        }
    }

    //subresourcerange必須自定義，這個設置太過於靈活
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

    inline uint32_t GetMipMapLevels(int width, int height) {
        // 计算mipmap等级
        // 通常把图片高宽缩小一半就是一级，直到缩不动
        // 先max找出高宽像素里比较大的，然后用log2计算可以被2除几次，再向下取整就是这张图可以缩小多少次了
        // 最后加1是因为原图也要一个等级
        return static_cast<uint32_t>(std::floor(std::log2(std::max(width, height))) + 1);
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
        vkQueueWaitIdle(queue); //在犹豫是否需要转换其变成Device的维度
        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }


    //因为这些操作需要使用到物理设备和逻辑设备和池，所以直接传入封装的对象
    inline void GenerateMipMaps(FrameWork::VulkanDevice &device, FrameWork::VulkanImage &image, VkQueue queue) {
        //检查图片格式是否支持
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(device.physicalDevice, image.format, &formatProperties);
        if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
            std::cerr << "Image format not support linear blitting." << std::endl;
        }
        auto cmd = beginSingleTimeCommands(device.logicalDevice, device.commandPool);
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = image.image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        //此处的barrier不影响图像的队列
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        int32_t mipWidth = image.extent.width;
        int32_t mipHeight = image.extent.height;

        for (uint32_t i = 1; i < image.mipLevels; i++) {
            //将0级布局转换成源
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            // 这个image的数据写入应该在这个Barrier之前完成
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            // 这个Barrier之后就可以读这个image的数据了
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(cmd,
                                 // 指定应该在Barrier之前完成的操作，在管线里的哪个stage
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 // 指定应该等待Barrier的操作，在管线里的哪个stage
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 0, 0, nullptr, 0, nullptr, 1, &barrier
            );

            // 配置Blit操作，整个Blit操作就是把同一个image第i-1级mipmap的数据缩小一半复制到第i级
                VkImageBlit blit{};
                // 操作原图像的(0,0)到(width, height)
                blit.srcOffsets[0] = { 0, 0, 0 };
                blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
                // 操作原图像的Color
                blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                // 操作原图像mipmap等级的i-1
                blit.srcSubresource.mipLevel = i - 1;
                // 暂时没用
                blit.srcSubresource.baseArrayLayer = 0;
                blit.srcSubresource.layerCount = 1;
                // 复制到目标图像的(0,0)到(width/2, height/2)，如果小于1的话等于1
                blit.dstOffsets[0] = { 0, 0, 0 };
                blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
                // 操作目标图像的Color
                blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                // 复制到目标图像的mipmap等级i
                blit.dstSubresource.mipLevel = i;
                // 暂时没用
                blit.dstSubresource.baseArrayLayer = 0;
                blit.dstSubresource.layerCount = 1;

                // 添加Bilt操作指令，这里原图像和目标图像设置为同一个，因为是同一个image的不同mipmap层操作
                vkCmdBlitImage(cmd,
                    image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1, &blit, VK_FILTER_LINEAR
                );

                // Blit完之后，这个Barrier所对应的i-1级mipmap就结束任务了，可以提供给shader读取了
                // 所以layout从TRANSFER_SRC_OPTIMAL转换到SHADER_READ_ONLY_OPTIMAL
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                // 这个image第i-1级mipmap的数据读取操作应该在这个Barrier之前完成
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                vkCmdPipelineBarrier(cmd,
                    // 结合前面的srcAccessMask
                    // transfer阶段的transfer读取操作应该在这个Barrier之前执行
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    // 结合前面的dstAccessMask
                    // fragment shader阶段的shader读取操作应该在这个Barrier之后执行
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    0, 0, nullptr, 0, nullptr, 1, &barrier
                );

                if (mipWidth > 1) mipWidth /= 2;
                if (mipHeight > 1) mipHeight /= 2;
            }

            // 循环结束后还有最后一级的mipmap需要处理
            barrier.subresourceRange.baseMipLevel = image.mipLevels - 1;
            // 因为最后一级只接收数据，不需要从它复制数据到其它地方，所以最后的layout就是TRANSFER_DST_OPTIMAL
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            // 需要转换成shader读取用的SHADER_READ_ONLY_OPTIMAL
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            // 这个Barrier之前需要完成最后一级mipmap的数据写入
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            // shader读取数据需要在这个Barrier之后才能开始
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(cmd,
                // 结合前面的srcAccessMask
                // transfer阶段的transfer写入操作应该在这个Barrier之前执行
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                // 结合前面的dstAccessMask
                // fragment shader阶段的读取操作需要在这个Barrier之后才能开始
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                0, 0, nullptr, 0, nullptr, 1, &barrier
            );
        endSingleTimeCommands(device, queue, device.commandPool, cmd);
    }

    //类型Id获取器
    template<typename Category>
    struct IndexGetter {
        template<typename T>
        static uint32_t Get() {
            static uint32_t id = index_++;
            return id;
        }
    private:
        inline static uint32_t index_ = 0;
    };

}

#endif //VULKANTOOL_H
