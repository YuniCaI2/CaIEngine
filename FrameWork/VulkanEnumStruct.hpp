#pragma once

//使用glfw不需要手动加载vulkan的头文件
#define VK_NO_PROTOTYPES
#include<volk.h>
#include<vector>
#define GLFW_INCLUDE_VULKAN
#include<GLFW/glfw3.h>

//AMD 内存分布器
#include <vk_mem_alloc.h>

#include "pbh.hpp"


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
        VkDeviceSize size = 0;//Uniform Buffer's size
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
        FrameBufferType bufferType = FrameBufferType::Normal;
        RenderPassType renderPassType = RenderPassType::Normal;
    };

    struct VulkanDrawRecord{
        uint32_t VAO = 0;
        uint32_t pipelineID = 0;
        uint32_t materialDataID = 0;
        uint32_t instanceNum = 0;
        uint32_t instanceBuffer = UINT32_MAX;

        VulkanDrawRecord(uint32_t _VAO, uint32_t _pipelineID, uint32_t _materialDataID, uint32_t _instanceNum, uint32_t _instanceBuffer) 
            : VAO(_VAO), pipelineID(_pipelineID), materialDataID(_materialDataID), instanceNum(_instanceNum), instanceBuffer(_instanceBuffer) {}
    };

    struct VulkanASInstanceData{
        uint32_t VAO = 0;
        uint32_t hitGroupIdx = 0;
        uint32_t rtMaterialDataID = 0;
        glm::mat4 transform = glm::mat4(1.0f);
    };//这个是为了实现光线追踪加速

    struct VulkanCommand{
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        std::vector<VkSemaphore> signalSemphores;
    };

    struct VulkanDrawCommand{
        CommandType commandType = CommandType::NotCare;
        FrameBufferClearFlags clearFlags = CAI_CLEAR_FRAME_BUFFER_NONE_BIT;
        std::vector<VulkanCommand> drawCommands;
        bool inUse = false;
    };
    struct VulkanAccelerationStructure{
        bool isBuilt = false;
        VulkanBuffer buffer;
        VkDeviceAddress deviceAddress = 0;//GPU可直接访问的内存地址
        VkAccelerationStructureKHR as = VK_NULL_HANDLE; 
    };

    struct VulkanASGroup{
        std::vector<VulkanAccelerationStructure> asGroup;
        bool isUsed = false;
    };

    struct VulkanVAO{
        uint32_t indexCount = 0;
        VkBuffer indexBuffer = VK_NULL_HANDLE;
        VmaAllocation indexBufferAlloc  = nullptr;
        void* indexBufferAddress = nullptr; //Only for dynamic mesh
        VkDeviceAddress indexBufferDeviceAddress = 0; //GPU可直接访问的内存地址 Only for ray tracing

        uint32_t vertexCount = 0;
        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VmaAllocation vertexBufferAlloc = nullptr;
        void* vertexBufferAddress = nullptr; //Only for dynamic mesh
        VkDeviceAddress vertexBufferDeviceAddress = 0; //GPU可直接访问的内存地址 Only for ray tracing

        bool computeSkinned = false;
        std::vector<VulkanBuffer> ssbo;//缓冲区对象 

        VulkanAccelerationStructure blas; // Bottom Level Acceleration Structure
        bool isUsed = false;
    };

    struct VulkanSSBO{
        GPUBufferType bufferType = GPUBufferType::Static;
        std::vector<VulkanBuffer> buffers;
        uint32_t binding = 0;
        bool inUse = false;
    };

    struct VulkanPipeline{
        std::string name;
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout sceneDescriptorSetLayout = VK_NULL_HANDLE;// For ray tracing
    };

    struct VulkanMaterialData{
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> descriptorSets;
        std::vector<UniformBuffer> vertUniformBuffers;
        std::vector<UniformBuffer> geomUniformBuffers;
        std::vector<UniformBuffer> fragUniformBuffers;
        bool inUse = false;
    };

    struct VulkanRTPipelineData{
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> descriptorSets;
    };

    struct VulkanShaderBindingTable{
        VulkanBuffer buffer;//包含了着色器组的句柄
        VkStridedDeviceAddressRegionKHR raygenRegion = {};
        VkStridedDeviceAddressRegionKHR missRegion = {};//未命中调用的着色器
        VkStridedDeviceAddressRegionKHR hitRegion = {}; 
        VkStridedDeviceAddressRegionKHR callableRegion = {};
    };//光线追踪的着色器绑定表

    struct VulkanRTSceneData{
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> descriptorSets;
        std::vector<VulkanBuffer> dataReferenceBuffers;
    };
    
}