//
// Created by 51092 on 25-6-2.
//

#include "VulkanImage.h"

VkResult FrameWork::VulkanImage::map(VkDeviceSize size, VkDeviceSize offset) {
    return vkMapMemory(device, memory, offset, size, 0, &mapped);
}

void FrameWork::VulkanImage::unmap() {
    if (mapped) {
        vkUnmapMemory(device, memory);
        mapped = nullptr;
    }
}

VkResult FrameWork::VulkanImage::bind(VkDeviceSize offset) {
    return vkBindImageMemory(device, image, memory, offset);
}

void FrameWork::VulkanImage::copyTo(void* data, VkDeviceSize size) {
    assert(mapped);
    memcpy(mapped, data, size);
}



//CPU->GPU 使得GPU可见
VkResult FrameWork::VulkanImage::flush(VkDeviceSize size, VkDeviceSize offset) {
    VkMappedMemoryRange mappedMemoryRange = {};
    mappedMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedMemoryRange.memory = memory;
    mappedMemoryRange.offset = offset;
    return vkFlushMappedMemoryRanges(device, 1, &mappedMemoryRange);
}

//当内存不是主机一致的(host-coherent) 且GPU写入内存时，在CPU读取前需要调用此函数使GPU的修改对CPU可见。
VkResult FrameWork::VulkanImage::invalidate(VkDeviceSize size, VkDeviceSize offset) {
    VkMappedMemoryRange mappedMemoryRange = {};
    mappedMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedMemoryRange.memory = memory;
    mappedMemoryRange.offset = offset;
    mappedMemoryRange.size = size;
    return vkInvalidateMappedMemoryRanges(device, 1, &mappedMemoryRange);
}

void FrameWork::VulkanImage::destroy() {
    //保证图片可以删除
    if (image) {
        vkDestroyImage(device, image, nullptr);
        image = VK_NULL_HANDLE;
    }
    if (memory) {
        vkFreeMemory(device, memory, nullptr);
        memory = VK_NULL_HANDLE;
    }
}
