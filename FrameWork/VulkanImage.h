//
// Created by 51092 on 25-6-2.
//

#ifndef VULKANIMAGE_H
#define VULKANIMAGE_H
#include "pubh.h"

//ImageView 相当于对Image的一种解释这里不将其封装在一起
//当然这里没有包括3D纹理，只有2D纹理，3D纹理准备单独封装
namespace FrameWork {
    class VulkanImage {
    public:
        VkDevice device;
        VkImage image;
        VkExtent2D extent;
        uint32_t mipLevels;
        uint32_t arrayLayers;
        VkSampleCountFlagBits samples; //采样数
        VkFormat format;
        VkImageTiling tiling;
        VkImageUsageFlags usage{};

        VkDeviceMemory memory;
        //这里为动态做准备
        void* mapped {nullptr};
        //一些特性供后续查询
        VkMemoryPropertyFlags properties{};

        VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        void unmap();

        VkResult bind(VkDeviceSize offset = 0);
        void copyTo(void* data, VkDeviceSize size);
        VkResult flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        VkResult invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        void destroy();
    };
}



#endif //VULKANIMAGE_H
