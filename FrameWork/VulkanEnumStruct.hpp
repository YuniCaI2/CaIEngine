#pragma once

//使用glfw不需要手动加载vulkan的头文件
#include <sys/types.h>
#define VK_NO_PROTOTYPES
#include<volk.h>
#include<vector>
#define GLFW_INCLUDE_VULKAN
#include<GLFW/glfw3.h>

//AMD 内存分布器
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>


#define ShaderModeleSet  std::map<VkShaderStageFlagBits, VkShaderModule>

namespace CaIEngine {
    const uint32_t MAX_FRAMES_IN_FLIGHT = 1; //飞行帧的数量

    extern bool vkIsSupportPortablitySubset;//支持可移植性子集——适用在MacOS上

    //需要验证层代码
    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    //需要扩展的代码
    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        "VK_KHR_portability_subset"
    };

    //光线追踪的扩展 ——我不确定可以跨平台
    const std::vector<const char*> rayTracingExtensions = {
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_KHR_SHADER_CLOCK_EXTENSION_NAME
    };

    enum class RenderPassType{
        Present,
        PresentOverspread,
        Normal,
        Color,
        ShadowMap,
        ShadowCubeMap,
        GBuffer,
        Deferred,
        MAX
    };

    struct QueueFamilyIndices {
        uint32_t present = UINT32_MAX;
        uint32_t graphics = UINT32_MAX;
        uint32_t compute = UINT32_MAX;
        
        //默认支持计算着色器
        bool isComplete() {
            return present != UINT32_MAX && graphics != UINT32_MAX && compute != UINT32_MAX;
        }
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities = {};
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    struct VulkanBuffer{
        
        VkBuffer buffer = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        void* mappedAddress = nullptr;
        VkDeviceAddress deviceAddress = 0;
        bool inUse = false;
    };

    struct UniformBuffer{
        uint32_t binding = 0;
        VkDeviceSize size = 0;
        VulkanBuffer buffer;
        void* mappedAddress = nullptr;
    };

    struct VulkanImage{
        VkImage image = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;   
    };

    struct VulkanTexture{
        VulkanImage image;
        VkImageView imageView = VK_NULL_HANDLE;
        VkSampler sampler = VK_NULL_HANDLE;
        bool inUse = false;
    };

    struct VulkanFBO{
        std::vector<VkFramebuffer> framebuffers;
        uint32_t colorAttachmentIdx = UINT32_MAX;
        uint32_t depthAttachmentIdx = UINT32_MAX;
        uint32_t positionAttachmentIdx = UINT32_MAX;
        uint32_t normalAttachmentIdx = UINT32_MAX;
    };

}