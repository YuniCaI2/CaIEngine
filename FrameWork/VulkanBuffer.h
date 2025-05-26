//
// Created by 51092 on 25-5-10.
//

#ifndef VULKANBUFFER_H
#define VULKANBUFFER_H
#include "pubh.h"

//5.10 这个类好像不支持传输，这个好像是主机可访问的
namespace FrameWork {
    class Buffer {
    public:
        VkDevice device{};
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkDescriptorBufferInfo descriptor{};
        VkDeviceSize size = 0;
        VkDeviceSize alignment = 0;
        void* mapped = nullptr;
        //缓存区创建时，由外部填写的使用标志,以便在后面可以查询
        VkBufferUsageFlags usageFlags{};
        //缓冲区创建时，由外部填写的属性标志，以便在后面可以查询
        VkMemoryPropertyFlags memoryPropertyFlags{};
        VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        void unmap();
        VkResult bind(VkDeviceSize offset = 0);
        void setupDescriptor(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        void copyTo(void* data, VkDeviceSize size);
        //VK_WHOLE_SIZE是在不确定缓冲区大小时，可以覆盖整个缓冲区
        VkResult flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        VkResult invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        void destroy();

    };
}



#endif //VULKANBUFFER_H
