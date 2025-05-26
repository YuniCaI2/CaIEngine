//
// Created by 51092 on 25-5-10.
//

#include "VulkanBuffer.h"

#include <assert.h>
#include <cstring>

//设备内存映射到主机可见
VkResult FrameWork::Buffer::map(VkDeviceSize size, VkDeviceSize offset) {
    return vkMapMemory(device, memory, offset, size, 0, &mapped);
}

//这里不设置返回VkResult
void FrameWork::Buffer::unmap() {
    if (mapped) {
        vkUnmapMemory(device, memory);
        mapped = nullptr;       
    }
}

//offset是可选的
VkResult FrameWork::Buffer::bind(VkDeviceSize offset) {
    return vkBindBufferMemory(device, buffer, memory, offset);   
}

//设置默认的描述符
void FrameWork::Buffer::setupDescriptor(VkDeviceSize size, VkDeviceSize offset) {
    descriptor.buffer = buffer;
    descriptor.offset = offset;
    descriptor.range = size;   
}

//主机内存复制到映射的设备缓冲区
void FrameWork::Buffer::copyTo(void *data, VkDeviceSize size) {
    assert(mapped);
    memcpy(mapped, data, size);
}

//当内存不是主机一致（host-coherent）时，需要显式调用此函数使 CPU 的修改对 GPU 可见。这通常在更新内存后、GPU 读取之前调用.
VkResult FrameWork::Buffer::flush(VkDeviceSize size, VkDeviceSize offset) {
    VkMappedMemoryRange range = {};
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.memory = memory;
    range.offset = offset;
    range.size = size;
    return vkFlushMappedMemoryRanges(device, 1, &range);
}

//当内存不是主机一致（host-coherent）且 GPU 写入了内存时，在 CPU 读取前需要调用此函数使 GPU 的修改对 CPU 可见。例如，当使用缓冲区存储着色器的输出结果时。
VkResult FrameWork::Buffer::invalidate(VkDeviceSize size, VkDeviceSize offset) {
    VkMappedMemoryRange range = {};
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.memory = memory;
    range.offset = offset;
    range.size = size;
    return vkInvalidateMappedMemoryRanges(device, 1, &range);
}

void FrameWork::Buffer::destroy() {
    if (buffer) {
        vkDestroyBuffer(device, buffer, nullptr);
        buffer = nullptr;
    }
    if (memory) {
        vkFreeMemory(device, memory, nullptr);
        memory = nullptr;
    }
}
