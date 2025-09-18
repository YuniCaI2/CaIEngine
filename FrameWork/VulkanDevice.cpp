//
// Created by 51092 on 25-5-10.
//

#include "VulkanDevice.h"
#include <assert.h>
#include <format>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include "VulkanTool.h"


FrameWork::VulkanDevice::VulkanDevice(VkPhysicalDevice physicalDevice, VkInstance instance) {
    assert(physicalDevice);
    this->physicalDevice = physicalDevice;
    this->instance = instance;

    //存储属性特征
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);
    vkGetPhysicalDeviceFeatures(physicalDevice, &features);
    //内存特征
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
    //队列家族
    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    queueFamilyProperties.resize(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
    if (extensionCount > 0) {
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, &availableExtensions.front())
            == VK_SUCCESS)
            for (const auto &extension: availableExtensions) {
                supportedExtensions.emplace_back(extension.extensionName);
            }
    }
}

FrameWork::VulkanDevice::~VulkanDevice() {
    if (commandPool) {
        vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
    }
    if (logicalDevice) {
        vkDestroyDevice(logicalDevice, nullptr);
    }
}

//typeBits: 资源所支持的内存类型的位掩码（from VkMemoryRequirements）
//properties: 内存属性的位掩码
//memTypeFound: 是一个可选值
uint32_t FrameWork::VulkanDevice::getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties,
                                                VkBool32 *memTypeFound) const {
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
        if ((typeBits & 1) == 1) {
            if ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                if (memTypeFound) {
                    *memTypeFound = VK_TRUE;
                }
                return i;
            }
        }
        typeBits >>= 1;
    }
    if (memTypeFound) {
        *memTypeFound = VK_FALSE;
        return 0;
    } else {
        throw std::runtime_error("failed to find suitable memory type!");
    }
}

uint32_t FrameWork::VulkanDevice::getQueueFamilyIndex(VkQueueFlagBits flags) const {
    //找到一个仅支持计算的队列——在一些情况下，图形队列和计算队列是一致的
    if ((flags & VK_QUEUE_COMPUTE_BIT) == flags) {
        for (uint32_t i = 0; i < queueFamilyProperties.size(); i++) {
            if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) && (
                    queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) {
                return i;
            }
        }
    }

    //找到一个仅传输队列
    if ((flags & VK_QUEUE_TRANSFER_BIT) == flags) {
        for (uint32_t i = 0; i < queueFamilyProperties.size(); i++) {
            if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) && (
                    queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) {
                return i;
            }
        }
    }

    for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++) {
        if ((queueFamilyProperties[i].queueFlags & flags) == flags) {
            return i;
        }
    }

    throw std::runtime_error("failed to find a queue family that supports the requested queue flags!");
}

//enableFeatures: 设备创建时支持的某些功能
//pNext:可选的扩展
//useSwapchain :是否支持交换链扩展
//requestedQueueTypes: 指定要从设备请求的队列类型
VkResult FrameWork::VulkanDevice::createLogicalDevice(VkPhysicalDeviceFeatures enabledFeatures,
                                                      std::vector<const char *> enabledExtensions, void *pNextChain,
                                                      bool useSwapChain, VkQueueFlags requestQueueType) {
    //逻辑设备创建时需要请求所需的队列

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};

    //获得对应类型的队列家族序号
    //索引可能会发生重叠

    const float defaultQueuePriority = 0.0f;

    //图形队列
    if (requestQueueType & VK_QUEUE_GRAPHICS_BIT) {
        queueFamilyIndices.graphics = getQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamilyIndices.graphics;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &defaultQueuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    } else {
        queueFamilyIndices.graphics = 0;
    }

    //专用计管线
    if (requestQueueType & VK_QUEUE_COMPUTE_BIT) {
        queueFamilyIndices.compute = getQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT);

        if (queueFamilyIndices.compute != queueFamilyIndices.graphics) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamilyIndices.compute;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &defaultQueuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }
    } else {
        queueFamilyIndices.compute = queueFamilyIndices.graphics;
        //和图形管线一致
    }

    //专有的传输队列
    if (requestQueueType & VK_QUEUE_TRANSFER_BIT) {
        queueFamilyIndices.transfer = getQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT);
        if ((queueFamilyIndices.transfer != queueFamilyIndices.graphics) && (
                queueFamilyIndices.transfer != queueFamilyIndices.compute)) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamilyIndices.transfer;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &defaultQueuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }
    } else {
        queueFamilyIndices.transfer = queueFamilyIndices.graphics;
    }

    //创建逻辑设备
    std::vector<const char *> deviceExtensions(std::move(enabledExtensions));
    if (useSwapChain) {
        deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
#if defined(_WIN32)
        deviceExtensions.push_back(VK_EXT_BLEND_OPERATION_ADVANCED_EXTENSION_NAME);
#endif
    }

    //保证Apple扩展支持
#if defined(__APPLE__) && defined(__MACH__)
        deviceExtensions.push_back("VK_KHR_portability_subset");
#endif

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &enabledFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());



    // 如果有pNext
    VkPhysicalDeviceFeatures2 features2{};
    if (pNextChain) {
        features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        features2.pNext = pNextChain;
        features2.features = enabledFeatures;
        createInfo.pNext = &features2;
        createInfo.pEnabledFeatures = nullptr;
    }

    if (deviceExtensions.size() > 0) {
        for (const char *enableExtension: deviceExtensions) {
            if (!extensionsSupported(enableExtension)) {
                std::cerr << "failed to find required extension: " << enableExtension << std::endl;
            }
        }

        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    }

    this->enabledFeatures = enabledFeatures;
    VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice);
    if (result != VK_SUCCESS) {
        return result;
    }

    //创建默认的命令池
    commandPool = createCommandPool(queueFamilyIndices.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    return result;
}

//创建缓冲
// Buffer 的使用标志，指定资源的使用预期
// 内存类型
//Buffer的尺寸
//buffer的指针
//内存的指针
//data
VkResult FrameWork::VulkanDevice::createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags,
                                    VkDeviceSize size, VkBuffer *buffer, VkDeviceMemory *memory, void *data) {
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usageFlags;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_CHECK_RESULT(vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, buffer));

    VkMemoryRequirements memRequirements;
    VkMemoryAllocateInfo allocInfo = {};
    vkGetBufferMemoryRequirements(logicalDevice, *buffer, &memRequirements);
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = getMemoryType(memRequirements.memoryTypeBits, memoryPropertyFlags);
    // If the buffer has VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT set we also need to enable the appropriate flag during allocation
    VkMemoryAllocateFlagsInfoKHR allocFlagsInfo{};
    if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
        allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
        allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
        allocInfo.pNext = &allocFlagsInfo;
    }
    VK_CHECK_RESULT(vkAllocateMemory(logicalDevice, &allocInfo, nullptr, memory));

    if (data != nullptr) {
        void* mapped;
        VK_CHECK_RESULT(vkMapMemory(logicalDevice, *memory, 0, size, 0, &mapped));
        memcpy(mapped, data, static_cast<size_t>(size));
        //没有设置内存主机一致性，请手动刷新使写入设备可见
        if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0) {
            VkMappedMemoryRange mappedMemoryRange = {};
            mappedMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            mappedMemoryRange.memory = *memory;
            mappedMemoryRange.offset = 0;
            mappedMemoryRange.size = size;
            vkFlushMappedMemoryRanges(logicalDevice, 1, &mappedMemoryRange);
        }
        vkUnmapMemory(logicalDevice, *memory);
    }

    //Attach the memory to the buffer object
    VK_CHECK_RESULT(vkBindBufferMemory(logicalDevice, *buffer, *memory, 0));

    return VK_SUCCESS;
}


VkResult FrameWork::VulkanDevice::createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags,
                                               Buffer *buffer, VkDeviceSize size, void *data) {
    buffer->device = logicalDevice;

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usageFlags;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VK_CHECK_RESULT(vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, &buffer->buffer));
    //创建内存
    VkMemoryRequirements memRequirements;
    VkMemoryAllocateInfo allocInfo = {};
    vkGetBufferMemoryRequirements(logicalDevice, buffer->buffer, &memRequirements);
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = getMemoryType(memRequirements.memoryTypeBits, memoryPropertyFlags);

    // If the buffer has VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT set we also need to enable the appropriate flag during allocation

    VkMemoryAllocateFlagsInfoKHR allocFlagsInfo = {};
    if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
        allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
        allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
        allocInfo.pNext = &allocFlagsInfo;
    }
    VK_CHECK_RESULT(vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &buffer->memory));

    buffer->size = size;
    buffer->alignment = memRequirements.alignment;
    buffer->usageFlags = usageFlags;
    buffer->memoryPropertyFlags = memoryPropertyFlags;

    if (data != nullptr) {
        VK_CHECK_RESULT(buffer->map());
        memcpy(buffer->mapped, data, size);
        if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0) {
            buffer->flush();
        }
        buffer->unmap();
    }

    //设置默认的描述符可以覆盖整个缓冲大小
    buffer->setupDescriptor();

    return buffer->bind();
}

void FrameWork::VulkanDevice::copyBuffer(Buffer *src, Buffer *dst, VkQueue queue, VkBufferCopy *copyRegion) {
    assert(dst->size <= src->size);
    assert(src->buffer);
    VkCommandBuffer copyBuffer = createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
    VkBufferCopy bufferCopy = {};
    if (copyRegion == nullptr) {
        bufferCopy.size = src->size;
    }else {
        bufferCopy = *copyRegion;
    }
    vkCmdCopyBuffer(copyBuffer, src->buffer, dst->buffer, 1, &bufferCopy);

    flushCommandBuffer(copyBuffer, queue);
}

VkResult FrameWork::VulkanDevice::createImage(VulkanImage* vulkanImage, VkExtent2D extent, uint32_t mipmapLevels, uint32_t arrayLayers,
    VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usageFlags,
    VkMemoryPropertyFlags memoryPropertyFlags) {
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = extent.width;
    imageInfo.extent.height = extent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipmapLevels;
    imageInfo.arrayLayers = arrayLayers;
    imageInfo.samples = numSamples;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;//只能被一个队列簇访问
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; //初始创建都为UNDEFINE 如果为别的初始状态可以使用tansition改变
    imageInfo.usage = usageFlags;
    imageInfo.flags = arrayLayers == 6 ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0; //支持天空盒的扩展

    vulkanImage->device = logicalDevice;
    vulkanImage->extent = extent;
    vulkanImage->mipLevels = mipmapLevels;
    vulkanImage->arrayLayers = arrayLayers;
    vulkanImage->format = format;
    vulkanImage->tiling = tiling;
    vulkanImage->usage = usageFlags;
    vulkanImage->samples = numSamples;

    VK_CHECK_RESULT(vkCreateImage(logicalDevice, &imageInfo, nullptr, &vulkanImage->image));

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(logicalDevice, vulkanImage->image, &memRequirements);
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = getMemoryType(memRequirements.memoryTypeBits, memoryPropertyFlags);

    // std::cout << "=== Allocation Debug ===" << std::endl;
    // std::cout << "Requested size: " << memRequirements.size << " bytes (" << memRequirements.size / 1024 / 1024 << " MB)" << std::endl;
    // std::cout << "Memory type index: " << allocInfo.memoryTypeIndex << std::endl;

    VK_CHECK_RESULT(vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &vulkanImage->memory));

    return vkBindImageMemory(logicalDevice, vulkanImage->image, vulkanImage->memory, 0);
}

VkResult FrameWork::VulkanDevice::copyBufferToImage(Buffer *src, VulkanImage *dst,VkImageLayout imageLayout,//imageLayout是当前图片的布局
    VkQueue queue,VkBufferImageCopy *copyRegion) {
    if (copyRegion == nullptr) {
        VkBufferImageCopy region{};
        // 从buffer读取数据的起始偏移量
        region.bufferOffset = 0;
        // 这两个参数明确像素在内存里的布局方式，如果我们只是简单的紧密排列数据，就填0
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        // 下面4个参数都是在设置我们要把数据拷贝到image的哪一部分
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;//一般不会有深度图需要拷贝
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        // 这个也是在设置我们要把图像拷贝到哪一部分
        // 如果是整张图片，offset就全是0，extent就直接是图像高宽
        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { (uint32_t)dst->extent.width, (uint32_t)dst->extent.height, 1 };

        auto imCommandBuffer = VulkanTool::beginSingleTimeCommands(logicalDevice, commandPool);
        vkCmdCopyBufferToImage(imCommandBuffer, src->buffer, dst->image, imageLayout, 1, &region);
        VulkanTool::endSingleTimeCommands(logicalDevice, queue,commandPool, imCommandBuffer);

    }else {
        if (commandPool == VK_NULL_HANDLE) {
            std::cerr << "Command pool is null, you should create logical device first !" << std::endl;
            exit(-1);
        }
        auto imCommandBuffer = VulkanTool::beginSingleTimeCommands(logicalDevice, commandPool);
        vkCmdCopyBufferToImage(imCommandBuffer, src->buffer, dst->image, imageLayout, 1, copyRegion);
        VulkanTool::endSingleTimeCommands(logicalDevice, queue,commandPool, imCommandBuffer);
    }
    return VK_SUCCESS;
}


VkCommandPool FrameWork::VulkanDevice::createCommandPool(uint32_t queueFamilyIndex,
                                                         VkCommandPoolCreateFlags createFlags) {

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndex;
    poolInfo.flags = createFlags;
    VkCommandPool pool;
    VK_CHECK_RESULT(vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &pool));
    return pool;
}

//begin为真则在新的缓冲区上录制
VkCommandBuffer FrameWork::VulkanDevice::
createCommandBuffer(VkCommandBufferLevel level, VkCommandPool pool, bool begin) {

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = pool;
    allocInfo.level = level;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    VK_CHECK_RESULT(vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffer));

    if (begin) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &beginInfo));
    }
    return commandBuffer;
}

VkCommandBuffer FrameWork::VulkanDevice::createCommandBuffer(VkCommandBufferLevel level, bool begin) {
    return createCommandBuffer(level, commandPool, begin);
}

void FrameWork::VulkanDevice::flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, VkCommandPool pool,
                                                 bool free) {
    if (commandBuffer == VK_NULL_HANDLE) {
        return;
    }
    VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = 0;
    VkFence fence;
    VK_CHECK_RESULT(vkCreateFence(logicalDevice, &fenceInfo, nullptr, &fence));
    VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
    VK_CHECK_RESULT(vkWaitForFences(logicalDevice, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));
    //等待所有指定的Fence
    vkDestroyFence(logicalDevice, fence, nullptr);
    if (free) {
        vkFreeCommandBuffers(logicalDevice, pool, 1, &commandBuffer);
    }
}

void FrameWork::VulkanDevice::flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free) {
    flushCommandBuffer(commandBuffer, queue, commandPool, free);
}

bool FrameWork::VulkanDevice::extensionsSupported(std::string extension) {
    return (std::find(supportedExtensions.begin(), supportedExtensions.end(), extension) != supportedExtensions.end());
}

VkFormat FrameWork::VulkanDevice::getSupportedDepthFormat(bool checkSamplingSupport) {
    // All depth formats may be optional, so we need to find a suitable depth format to use
    std::vector<VkFormat> depthFormats = { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D16_UNORM };
    for (auto& format : depthFormats)
    {
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);
        // Format must support depth stencil attachment for optimal tiling
        if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
            if (checkSamplingSupport) {
                if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) {
                    continue;
                }
            }
            return format;
        }
    }
    throw std::runtime_error("Could not find a matching depth format");
}
