//
// Created by 51092 on 25-5-6.
//

#include "vulkanFrameWork.h"

#include <iostream>
#include <ostream>
#include<algorithm>

#include <array>
#include <vector>

#include "CompMaterial.h"
#include "CompShader.h"
#include "Logger.h"
#include "ShaderParse.h"
#include "VulkanDebug.h"
#include "VulkanTool.h"
#include "VulkanWindow.h"
#include "FrameGraph/ResourceManager.h"
#define VOLK_IMPLEMENTATION
#define _VALIDATION

std::string vulkanFrameWork::getWindowTitle() const {
    std::string windowTitle = {title + "-" + deviceProperties.deviceName};
    if (!settings.overlay) {
        windowTitle += "-" + std::to_string(frameCounter) + "fps";
        windowTitle += "-frameTimer:";
        windowTitle += std::to_string(frameTimer);
    }
    return windowTitle;
}

void vulkanFrameWork::createPipelineCache() {
    VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
    pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    VK_CHECK_RESULT(vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &pipelineCache));
}

//信号量在initVulkan中已经进行了设置
void vulkanFrameWork::createSynchronizationPrimitives() {
    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    //Fence
    waitFences.resize(MAX_FRAME);
    for (auto & waitFence : waitFences) {
        VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &waitFence));
    }
    waitFences.resize(MAX_FRAME);
}

void vulkanFrameWork::createSurface() {
    swapChain.initSurface(window);
}

void vulkanFrameWork::createSwapChain() {
    //这是为了解决Mac的Retina屏幕问题
    glfwGetFramebufferSize(window, reinterpret_cast<int*>(&windowWidth), reinterpret_cast<int*>(&windowHeight));
    swapChain.create(windowWidth, windowHeight, settings.vsync, settings.fullscreen);



}

void vulkanFrameWork::CreateSwapChainTex() {
    //准备swapchainTex
    swapChainTextures.resize(swapChain.images.size());
    for (int i = 0; i < swapChain.images.size(); i++) {
        swapChainTextures[i] = getNextIndex<FrameWork::Texture>();
        auto texture = getByIndex<FrameWork::Texture>(swapChainTextures[i]);
        texture->imageView = swapChain.imageViews[i];
        texture->image.image = swapChain.images[i];
        texture->image.layout = VK_IMAGE_LAYOUT_UNDEFINED;
        texture->image.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        texture->image.format = swapChain.colorFormat;
        texture->isSwapChainRef = true;
        texture->inUse = true;
    }
}

void vulkanFrameWork::createCommandBuffers() {
    //创建一个对每个交换链图片创建命令缓冲 (可能是为了飞行帧)
    drawCmdBuffers.resize(swapChain.images.size());
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(drawCmdBuffers.size());
    VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &allocInfo, drawCmdBuffers.data()));
}

void vulkanFrameWork::destroyCommandBuffers() {
    vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(drawCmdBuffers.size()), drawCmdBuffers.data());
}

std::string vulkanFrameWork::getShaderPath() const {
    return VulkanTool::getShaderBasePath() + shaderDir + "/";
}

vulkanFrameWork::vulkanFrameWork() {
}

// vulkanFrameWork::~vulkanFrameWork() {
//     //删除池中的内容,完全释放
//     //高层向低层释放
//     std::cerr << (int)(device == VK_NULL_HANDLE) << std::endl;
//
//
//     for (int i = 0; i < vulkanPipelineInfos.size(); i++) {
//         if (vulkanPipelineInfos[i] != nullptr) {
//             destroyByIndex<FrameWork::VulkanPipelineInfo>(i);
//             delete vulkanPipelineInfos[i];
//             vulkanPipelineInfos[i] = nullptr;
//         }
//     }
//
//     for (int i = 0; i < materials.size(); i++) {
//         if (materials[i] != nullptr) {
//             destroyByIndex<FrameWork::Material>(i);
//             delete materials[i];
//             materials[i] = nullptr;
//         }
//     }
//     for (int i = 0; i < vulkanFBOs.size(); i++) {
//         if (vulkanFBOs[i] != nullptr) {
//             destroyByIndex<FrameWork::VulkanFBO>(i);
//             delete vulkanFBOs[i];
//             vulkanFBOs[i] = nullptr;
//         }
//     }
//     for (int i = 0; i < attachmentBuffers.size(); i++) {
//         if (attachmentBuffers[i] != nullptr) {
//             destroyByIndex<FrameWork::VulkanAttachment>(i);
//             delete attachmentBuffers[i];
//             attachmentBuffers[i] = nullptr;
//         }
//     }
//     for (int i = 0; i < textures.size(); i++) {
//         if (textures[i] != nullptr) {
//             destroyByIndex<FrameWork::Texture>(i);
//             delete textures[i];
//             textures[i] = nullptr;
//         }
//     }
//     for (int i = 0; i < meshes.size(); i++) {
//         if (meshes[i] != nullptr) {
//             destroyByIndex<FrameWork::Mesh>(i);
//             delete meshes[i];
//             meshes[i] = nullptr;
//         }
//     }
//     for (int i = 0; i < models.size(); i++) {
//         if (models[i] != nullptr) {
//             destroyByIndex<FrameWork::Model>(i);
//             delete models[i];
//             models[i] = nullptr;
//         }
//     }
//
//     for (int i = 0; i < vulkanPipelines.size(); i++) {
//         if (vulkanPipelines[i] != nullptr) {
//             destroyByIndex<FrameWork::VulkanPipeline>(i);
//             delete vulkanPipelines[i];
//             vulkanPipelines[i] = nullptr;
//         }
//     }
//     for (auto& [_, r] : renderPasses) {
//        if (r != VK_NULL_HANDLE) {
//            vkDestroyRenderPass(device, r, nullptr);
//        }
//     }
//     for (auto& [_, d] : descriptorSetLayouts) {
//         if (d != VK_NULL_HANDLE) {
//             vkDestroyDescriptorSetLayout(device, d, nullptr);
//         }
//     }
//     for (auto& slot : slots_) {
//         delete slot;
//     }
//     // Clean up Vulkan resources
//     swapChain.cleanup();
//     vulkanDescriptorPool.DestroyDescriptorPool();
//     if (descriptorPool != VK_NULL_HANDLE)
//     {
//         vkDestroyDescriptorPool(device, descriptorPool, nullptr);
//     }
//
//
//     for (auto& shaderModule : shaderModules)
//     {
//         vkDestroyShaderModule(device, shaderModule, nullptr);
//     }
//
//     vkDestroyPipelineCache(device, pipelineCache, nullptr);
//     for (auto& fence : waitFences) {
//         vkDestroyFence(device, fence, nullptr);
//     }
//
//
//     for (int i = 0 ; i < MAX_FRAME; i ++) {
//         vkDestroySemaphore(device, semaphores.presentComplete[i], nullptr);
//         vkDestroySemaphore(device, semaphores.renderComplete[i], nullptr);
//     }
//
//
//     delete vulkanDevice;
//
//     if (settings.validation)
//     {
//         FrameWork::VulkanDebug::freeDebugCallBack(instance);
//     }
//
//     vkDestroyInstance(instance, nullptr);
// }

bool vulkanFrameWork::initVulkan() {
    // Instead of checking for the command line switch, validation can be forced via a define
    //设置验证层
#ifdef _VALIDATION
    this->settings.validation = true;
#endif


    //设置实例
    VkResult result = createInstance();
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create Vulkan instance" << std::endl;
        return false;
    }

    if (settings.validation) {
        FrameWork::VulkanDebug::setDebugging(instance);
    }

    uint32_t gpuCount = 0;
    //得到可用的GPU的数量
    VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr));
    if (gpuCount == 0) {
        std::cerr << "Failed to find GPUs with Vulkan support" << std::endl;
        return false;
    }
    // Enumerate devices
    std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
    result = vkEnumeratePhysicalDevices(instance, &gpuCount, physicalDevices.data());
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to enumerate GPUs with Vulkan support" << std::endl;
        return false;
    }

    //评分函数
    auto scoreDevice = [](VkPhysicalDevice device, const VkPhysicalDeviceProperties& deviceProperties) {
        int score = 0;
        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            score += 1000;
        }else if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
            score += 500;
        }else {
            score += 200;
        }
        return score;
    };

    // 选择 GPU
    std::vector<std::pair<VkPhysicalDevice, int>> deviceScores;
    for (const auto& p : physicalDevices) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(p, &properties);
        int score = 0;
        score = scoreDevice(p, properties);
        deviceScores.emplace_back(p, score);
    }
    std::sort(deviceScores.begin(), deviceScores.end(),
          [](const auto& a, const auto& b) {
              return a.second > b.second;
          });

    physicalDevice = deviceScores.front().first; //選擇分數最高的設備
    //选择独立显卡
    //再不济选择其他显卡

    //存储这些信息
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
    vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &deviceMemoryProperties);

    //检测物理设备是否支持dynamic Rendering
    VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures = {};
    dynamicRenderingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;

    VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.pNext = &dynamicRenderingFeatures;

    vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);
    
    if(! dynamicRenderingFeatures.dynamicRendering){
        LOG_ERROR("The Physical Device does not support dynamic rendering");
        return false;
    }


    //给派生类接口可以添加特性
    //设置各项异性
    if (deviceFeatures.samplerAnisotropy) {
        enabledFeatures.samplerAnisotropy = deviceFeatures.samplerAnisotropy;
        enabledFeatures.fillModeNonSolid = deviceFeatures.fillModeNonSolid;
    }
    if (settings.validation) {
        // enabledDeviceExtensions.push_back("VK_KHR_video_decode_queue");
        // enabledDeviceExtensions.push_back("VK_KHR_video_encode_queue");
        // enabledDeviceExtensions.push_back("VK_KHR_video_queue");
    }

    vulkanDevice = new FrameWork::VulkanDevice(physicalDevice, instance);

    //留给派生类的接口可以添加扩展
    getEnabledExtensions();
    // enabledDeviceExtensions = {
    //     "VK_KHR_dynamic_rendering"
    // };

    VkPhysicalDeviceVulkan13Features v13 = {};
    v13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    v13.dynamicRendering = VK_TRUE;

    deviceCreatepNextChain = &v13;

    VkPhysicalDeviceDynamicRenderingFeatures enabledDynamicRenderingFeatures = {};
    enabledDynamicRenderingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    enabledDynamicRenderingFeatures.dynamicRendering = VK_TRUE;

    //加入dynamic Rendering
    deviceCreatepNextChain = &enabledDynamicRenderingFeatures;

    result = vulkanDevice->createLogicalDevice(enabledFeatures, enabledDeviceExtensions, deviceCreatepNextChain);

    auto counts = deviceProperties.limits.framebufferColorSampleCounts & deviceProperties.limits.framebufferDepthSampleCounts;
    auto getMaxSampleCount = [](VkSampleCountFlags counts) {
        if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
        if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
        if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
        if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
        if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
        if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

        return VK_SAMPLE_COUNT_1_BIT;
    };

    msaaSamples = getMaxSampleCount(counts);

    commandPool = vulkanDevice->commandPool;
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create logical device" << std::endl;
        exit(-1);
    }
    device = vulkanDevice->logicalDevice;

    //获得图像队列的句柄
    //注意获得的句柄需要保证再device获取的时候存在
    graphicsQueue = VK_NULL_HANDLE;
    vkGetDeviceQueue(device, vulkanDevice->queueFamilyIndices.graphics, 0, &graphicsQueue);

    VkBool32 validFormat{false};

    //创建动态描述符池
    vulkanDescriptorPool.InitDescriptorPool(vulkanDevice);

    //获取合适的深度格式
    if (requiresStencil) {
        validFormat = VulkanTool::getSupportedDepthStencilFormat(physicalDevice, &depthFormat);
    }else {
        validFormat = VulkanTool::getSupportDepthFormat(physicalDevice, &depthFormat);
    }
    assert(validFormat);

    swapChain.setContext(instance, physicalDevice, device);

    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    //创建同步变量
    for (int i = 0 ; i < MAX_FRAME; i ++) {
        VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphores.presentComplete[i]));
        VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphores.renderComplete[i]));
    }

    setWindow();
    prepare();

    frameCountTimeStamp = 0;
    return true;
}




void vulkanFrameWork::DestroyAll() {

    //手动释放智能指针，因为API是手动释放的
    FrameWork::CaIShader::DestroyAll();
    FrameWork::CaIMaterial::DestroyAll();
    FrameWork::CompShader::DestroyAll();
    FrameWork::CompMaterial::DestroyAll();

    //释放CompMaterial
    for (int i = 0; i < compMaterialDatas_.size(); i++) {
        destroyByIndex<FrameWork::CompMaterialData>(i);
        delete compMaterialDatas_[i];
        compMaterialDatas_[i] = nullptr;
    }

    for (int i = 0; i < materialDatas_.size(); i++) {
        destroyByIndex<FrameWork::MaterialData>(i);
        delete materialDatas_[i];
        materialDatas_[i] = nullptr;
    }

    for (int i = 0; i < modelDatas_.size(); i++) {
        if (modelDatas_[i] != nullptr) {
            destroyByIndex<FrameWork::VulkanModelData>(i);
            delete modelDatas_[i];
            modelDatas_[i] = nullptr;
        }
    }

    for (int i = 0; i < vulkanPipelineInfos.size(); i++) {
        if (vulkanPipelineInfos[i] != nullptr) {
            destroyByIndex<FrameWork::VulkanPipelineInfo>(i);
            delete vulkanPipelineInfos[i];
            vulkanPipelineInfos[i] = nullptr;
        }
    }

    for (int i = 0; i < materials.size(); i++) {
        if (materials[i] != nullptr) {
            destroyByIndex<FrameWork::Material>(i);
            delete materials[i];
            materials[i] = nullptr;
        }
    }
    for (int i = 0; i < vulkanFBOs.size(); i++) {
        if (vulkanFBOs[i] != nullptr) {
            destroyByIndex<FrameWork::VulkanFBO>(i);
            delete vulkanFBOs[i];
            vulkanFBOs[i] = nullptr;
        }
    }
    for (int i = 0; i < attachmentBuffers.size(); i++) {
        if (attachmentBuffers[i] != nullptr) {
            destroyByIndex<FrameWork::VulkanAttachment>(i);
            delete attachmentBuffers[i];
            attachmentBuffers[i] = nullptr;
        }
    }
    for (int i = 0; i < textures.size(); i++) {
        if (textures[i] != nullptr) {
            destroyByIndex<FrameWork::Texture>(i);
            delete textures[i];
            textures[i] = nullptr;
        }
    }
    for (int i = 0; i < meshes.size(); i++) {
        if (meshes[i] != nullptr) {
            destroyByIndex<FrameWork::Mesh>(i);
            delete meshes[i];
            meshes[i] = nullptr;
        }
    }
    for (int i = 0; i < models.size(); i++) {
        if (models[i] != nullptr) {
            destroyByIndex<FrameWork::Model>(i);
            delete models[i];
            models[i] = nullptr;
        }
    }

    for (int i = 0; i < vulkanPipelines.size(); i++) {
        if (vulkanPipelines[i] != nullptr) {
            destroyByIndex<FrameWork::VulkanPipeline>(i);
            delete vulkanPipelines[i];
            vulkanPipelines[i] = nullptr;
        }
    }
    for (auto& [_, r] : renderPasses) {
       if (r != VK_NULL_HANDLE) {
           vkDestroyRenderPass(device, r, nullptr);
       }
    }



    //----------------

    // Clean up Vulkan resources
    swapChain.cleanup();
    vulkanDescriptorPool.DestroyDescriptorPool();
    if (descriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    }


    for (auto& shaderModule : shaderModules)
    {
        vkDestroyShaderModule(device, shaderModule, nullptr);
    }

    vkDestroyPipelineCache(device, pipelineCache, nullptr);
    for (auto& fence : waitFences) {
        vkDestroyFence(device, fence, nullptr);
    }


    for (int i = 0 ; i < MAX_FRAME; i ++) {
        vkDestroySemaphore(device, semaphores.presentComplete[i], nullptr);
        vkDestroySemaphore(device, semaphores.renderComplete[i], nullptr);
    }


    delete vulkanDevice;

    if (settings.validation)
    {
        FrameWork::VulkanDebug::freeDebugCallBack(instance);
    }

    vkDestroyInstance(instance, nullptr);
}

bool vulkanFrameWork::setWindow() {

    window = FrameWork::VulkanWindow::GetInstance().GetWindow();
    return true;
}

VkResult vulkanFrameWork::createInstance() {

    std::vector<const char *> instanceExtensions = {}; //新版本只需要在邏輯設備中設置就行了好像

    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        // 添加调试输出来检查GLFW扩展
    if (glfwExtensions == nullptr) {
        std::cerr << "Error: glfwGetRequiredInstanceExtensions returned null - Vulkan may not be supported or GLFW not initialized properly" << std::endl;
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    std::cout << "GLFW requires " << glfwExtensionCount << " Vulkan instance extensions:" << std::endl;
    for (uint32_t i = 0; i < glfwExtensionCount; i++) {
        std::cout << "  " << glfwExtensions[i] << std::endl;
    }

    instanceExtensions.insert(instanceExtensions.end(), glfwExtensions, glfwExtensions + glfwExtensionCount);

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    if (extensionCount > 0) {
        std::vector<VkExtensionProperties> availableExtensions(extensionCount); //这里面存储的对象不仅仅有名字还有支持的specVersion
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());
        for (const auto &extension: availableExtensions) {
            supportedInstanceExtensions.emplace_back(extension.extensionName);
        }
    }

    if (!enabledInstanceExtensions.empty()) {
        for (const auto* enableExtension : enabledInstanceExtensions) {
            if (std::find(supportedInstanceExtensions.begin(), supportedInstanceExtensions.end(), enableExtension) == supportedInstanceExtensions.end()) {
                std::cerr << "Error: " << enableExtension << " is not supported by the instance" << std::endl;
                return VK_ERROR_EXTENSION_NOT_PRESENT;
            }
            instanceExtensions.push_back(enableExtension);
        }
    }

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = name.c_str();
    appInfo.pEngineName = name.c_str();
    appInfo.apiVersion = apiVersion;

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;

    //保证跨平台
#if defined(_WIN32) || defined(_WIN64)
#elif defined(__APPLE__) && defined(__MACH__)
    instanceExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    instanceCreateInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
    if (settings.validation) {
        FrameWork::VulkanDebug::setupDebuggingMessengerCreateInfo(debugCreateInfo);
        debugCreateInfo.pNext = instanceCreateInfo.pNext; //保留原来的扩展链条
        instanceCreateInfo.pNext = &debugCreateInfo;//将新的添加了debug扩展的扩展链条添加
    }

    if (settings.validation || std::find(supportedInstanceExtensions.begin(), supportedInstanceExtensions.end(),
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME) != supportedInstanceExtensions.end()) {
        //保证扩展添加
        instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    if (!instanceExtensions.empty()) {
        instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
        instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
    }

    // The VK_LAYER_KHRONOS_validation contains all current validation functionality.
    //就是这一个层包括了所有
    //安卓需要NDK 20以上
    const char* validationLayerName = "VK_LAYER_KHRONOS_validation";

    if (settings.validation) {
        //Check
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
        bool layerFound = false;
        for (const auto &layer: availableLayers) {
            if (strcmp(layer.layerName, validationLayerName) == 0) {
                layerFound = true;
                break;
            }
        }
        if (layerFound) {
            instanceCreateInfo.enabledLayerCount = 1;
            instanceCreateInfo.ppEnabledLayerNames = &validationLayerName;
        }else {
            std::cerr << "Error: " << validationLayerName << " is not supported by the instance" << std::endl;
        }
    }

    //精确控制层的功能
    VkLayerSettingsCreateInfoEXT layerSettingsCreateInfo = {};
    layerSettingsCreateInfo.sType = VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT;
    if (!enabledLayerSettings.empty()) {
        layerSettingsCreateInfo.settingCount = static_cast<uint32_t>(enabledLayerSettings.size());
        layerSettingsCreateInfo.pSettings = enabledLayerSettings.data();
        //和上面的DeBug相同设置扩展链
        layerSettingsCreateInfo.pNext = instanceCreateInfo.pNext;
        instanceCreateInfo.pNext = &layerSettingsCreateInfo;
    }

    VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create Vulkan instance. Error code: " << result << std::endl;
        
        // 提供详细的错误信息
        switch (result) {
            case VK_ERROR_INCOMPATIBLE_DRIVER:
                std::cerr << "Incompatible driver" << std::endl;
                break;
            case VK_ERROR_EXTENSION_NOT_PRESENT:
                std::cerr << "Required extension not present" << std::endl;
                break;
            case VK_ERROR_LAYER_NOT_PRESENT:
                std::cerr << "Required layer not present" << std::endl;
                break;
            default:
                std::cerr << "Unknown error" << std::endl;
                break;
        }
        return result;
    }    

    //设置可捕获信息的验证层，并且对其进行初始化
    if (std::find(supportedInstanceExtensions.begin(), supportedInstanceExtensions.end(), VK_EXT_DEBUG_UTILS_EXTENSION_NAME) !=
        supportedInstanceExtensions.end()) {
        FrameWork::debugUtils::setup(instance);
    }
    return result;
}

void vulkanFrameWork::render() {

}



void vulkanFrameWork::CreateGPUBuffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, FrameWork::Buffer &buffer,
    void *data) {
    FrameWork::Buffer stagingBuffer;
    vulkanDevice->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &stagingBuffer, size, data);
    vulkanDevice->createBuffer(usageFlags | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &buffer, size, nullptr);
    vulkanDevice->copyBuffer(&stagingBuffer, &buffer, graphicsQueue);

    //删除中转内存
    stagingBuffer.destroy();
}

FrameWork::Buffer vulkanFrameWork::CreateUniformBuffer(const std::vector<FrameWork::ShaderProperty> &properties) {
    FrameWork::Buffer uniformBuffer{};
    if (properties.empty()) {
        LOG_ERROR("This Properties are empty. Can't create uniform buffer");
        return uniformBuffer;
    }
    uniformBuffer.size = static_cast<VkDeviceSize>(properties.back().offset + properties.back().size);
    vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniformBuffer,
        uniformBuffer.size, nullptr
        );
    uniformBuffer.map();
    return uniformBuffer;
}


// 这里之创建常规的纹理，不创建天空盒类型的纹理
void vulkanFrameWork::CreateTexture(uint32_t &textureId, const FrameWork::TextureFullData& data) {
    if (texturePathMap.find(data.path) != texturePathMap.end()) {
        //纹理资源复用
        textureId =  texturePathMap[data.path];
        return;
    }

    uint32_t pixelSize = 1;
    VkFormat  format = VK_FORMAT_R8G8B8A8_SRGB;
    if (data.numChannels == 3) {
        std::cerr << "Texture Channels are 3 that GPU is Not Supported" << std::endl;
    }
    if (data.numChannels == 4) {
        if (data.type == DiffuseColor || data.type == BaseColor) {
            format = VK_FORMAT_R8G8B8A8_SRGB;
        }
        else if (data.type == SFLOAT32) {
            format = VK_FORMAT_R32G32B32A32_SFLOAT;
            pixelSize = 4; //每一个像素的大小
        }
        else if (data.type == SFLOAT16) {
            format = VK_FORMAT_R16G16B16A16_SFLOAT;
            pixelSize = 2;//每一个像素的大小
        }
        else {
            format = VK_FORMAT_R8G8B8A8_UNORM;
        }
    }
    if (data.numChannels == 1) {
        format = VK_FORMAT_R8_UNORM;
    }
    //这里是为处理一些法线贴图和AO贴图等等，防止它们发生错误的空间映射

    FrameWork::Buffer stagingBuffer;
    vulkanDevice->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &stagingBuffer, data.width *data.height * data.numChannels * pixelSize , data.data);
    textureId = getNextIndex<FrameWork::Texture>();
    auto texture = getByIndex<FrameWork::Texture>(textureId);
    auto mipmapLevels = VulkanTool::GetMipMapLevels(data.width, data.height);
    //这里为了统一性array的加载只支持单层的，如果多层可以使用copy的方法叠起来
    vulkanDevice->createImage(&texture->image, VkExtent2D(data.width, data.height), mipmapLevels, 1,VK_SAMPLE_COUNT_1_BIT, format, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    auto cmd = VulkanTool::beginSingleTimeCommands(vulkanDevice->logicalDevice, vulkanDevice->commandPool);
    VkImageSubresourceRange subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = mipmapLevels,
        .baseArrayLayer = 0,
        .layerCount = 1
    };
    VulkanTool::setImageLayout(cmd, texture->image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    VulkanTool::endSingleTimeCommands(device, graphicsQueue, commandPool, cmd);

    vulkanDevice->copyBufferToImage(&stagingBuffer, &texture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, graphicsQueue);

    VulkanTool::GenerateMipMaps(*vulkanDevice, texture->image, graphicsQueue);

    //在生成mipmap的时候会将布局转换成着色器可读
    texture->image.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    stagingBuffer.destroy();

    CreateImageView(texture->image, texture->imageView, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D); //这里先保留旧接口
    texture->sampler = CreateSampler(mipmapLevels);
    texturePathMap[data.path] = textureId;
    texture->inUse = true;
    //使用
}


void vulkanFrameWork::CreateTexture(uint32_t &textureId, uint32_t& resolveID, FG::BaseDescription *description) { //特别的此函数只生成2D纹理， Cube纹理有CreateCubeTexture生成
    if (description->GetResourceType() != FG::ResourceType::Texture) {
        LOG_ERROR("Can't create resource which description is not texture type by create texture");
    }
    auto texDesc = static_cast<FG::TextureDescription*> (description);
    auto textureMipmap = texDesc->samples == VK_SAMPLE_COUNT_1_BIT ? texDesc->mipLevels : 1;
    textureId = getNextIndex<FrameWork::Texture>();
    auto texture = getByIndex<FrameWork::Texture>(textureId);
    vulkanDevice->createImage(&texture->image, VkExtent2D(texDesc->width, texDesc->height),
        textureMipmap, texDesc->arrayLayers,texDesc->samples, texDesc->format, VK_IMAGE_TILING_OPTIMAL,
    texDesc->usages, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (texDesc->usages & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        CreateImageView(texture->image, texture->imageView, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D);
    }else {
        CreateImageView(texture->image, texture->imageView, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D);
    }
    texture->image.layout = VK_IMAGE_LAYOUT_UNDEFINED;

    texture->sampler = CreateSampler(textureMipmap);
    //这里不关心纹理用来干什么，在FrameGraph直接可以通过Description区分
    texture->inUse = true;

    //对resolve处理
    if (texDesc->samples != VK_SAMPLE_COUNT_1_BIT) {
        resolveID = getNextIndex<FrameWork::Texture>();
        auto resolve = getByIndex<FrameWork::Texture>(resolveID);
        vulkanDevice->createImage(&resolve->image, VkExtent2D(texDesc->width, texDesc->height),
            texDesc->resolveMipLevels, texDesc->arrayLayers, VK_SAMPLE_COUNT_1_BIT, texDesc->format, VK_IMAGE_TILING_OPTIMAL,
        texDesc->usages , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (texDesc->usages & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            CreateImageView(resolve->image, resolve->imageView, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D);
        }else {
            CreateImageView(resolve->image, resolve->imageView, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D);
        }
        resolve->image.layout = VK_IMAGE_LAYOUT_UNDEFINED;
        resolve->sampler = CreateSampler(texDesc->resolveMipLevels);
        //这里不关心纹理用来干什么，在FrameGraph直接可以通过Description区分
        resolve->inUse = true;
        if (!(texDesc->usages & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) && texDesc->resolveMipLevels > 1) {
            //为各个Mipmap预先生成好对应的ImageView
            //因为多采样时原图不能有mipmap，所以此处对应resolve的mipmap
            resolve->mipMapViews.resize(texDesc->resolveMipLevels);//注意为了调用方便，所有Mipmap都生成一个imageView，所以第一个mipmap 0 和ImageView是重叠的
            for (uint32_t i = 0; i < texDesc->resolveMipLevels; i++) {
                VkImageSubresourceRange subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = i,
                    .levelCount = 1,
                    .baseArrayLayer = 0, //这里为单层的Image
                    .layerCount =  1
                };
                CreateImageView(resolve->image, resolve->mipMapViews[i], subresourceRange, VK_IMAGE_VIEW_TYPE_2D);
            }
        }
    }else {
        if (!(texDesc->usages & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) && texDesc->mipLevels > 1) {
            //为各个Mipmap预先生成好对应的ImageView
            texture->mipMapViews.resize(texDesc->mipLevels);//注意为了调用方便，所有Mipmap都生成一个imageView，所以第一个mipmap 0 和ImageView是重叠的
            for (uint32_t i = 0; i < texDesc->mipLevels; i++) {
                VkImageSubresourceRange subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = i,
                    .levelCount = 1,
                    .baseArrayLayer = 0, //这里为单层的Image
                    .layerCount =  1
                };
                CreateImageView(texture->image, texture->mipMapViews[i], subresourceRange, VK_IMAGE_VIEW_TYPE_2D);
            }
        }
    }//当采样数为1时

}

std::vector<uint32_t>& vulkanFrameWork::GetSwapChainTextures() {
    return swapChainTextures;
}


void vulkanFrameWork::CreateImageView(FrameWork::VulkanImage &image, VkImageView&imageView,
                                      VkImageAspectFlags aspectFlags, VkImageViewType viewType) {
    VkImageViewCreateInfo viewCreateInfo = {};
    viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCreateInfo.image = image.image;
    viewCreateInfo.viewType = viewType;
    viewCreateInfo.format = image.format;

    viewCreateInfo.subresourceRange.aspectMask = aspectFlags;
    viewCreateInfo.subresourceRange.baseMipLevel = 0;
    viewCreateInfo.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;
    viewCreateInfo.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    VK_CHECK_RESULT(vkCreateImageView(device, &viewCreateInfo, nullptr, &imageView));
}

void vulkanFrameWork::CreateImageView(FrameWork::VulkanImage &image, VkImageView&imageView,
    const VkImageSubresourceRange &subresourceRange, VkImageViewType viewType) {
    VkImageViewCreateInfo viewCreateInfo = {};
    viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCreateInfo.image = image.image;
    viewCreateInfo.viewType = viewType;
    viewCreateInfo.format = image.format;

    viewCreateInfo.subresourceRange = subresourceRange;

    viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;


    VK_CHECK_RESULT(vkCreateImageView(device, &viewCreateInfo, nullptr, &imageView));
}


void vulkanFrameWork::CreateAttachment(uint32_t &attachmentId, uint32_t width, uint32_t height, AttachmentType attachmentType, VkSampleCountFlagBits numSample, bool isSampled) {
    attachmentId = getNextIndex<FrameWork::VulkanAttachment>();
    auto attachment = getByIndex<FrameWork::VulkanAttachment>(attachmentId);
    attachment->type = attachmentType;
    attachment->width = width;
    attachment->height = height;
    attachment->isSampled = isSampled;
    attachment->samples = numSample;
    if (width == windowWidth && height == windowHeight)
        {attachment->isFollowWindow = true;}
    if (attachmentType == AttachmentType::Depth) {
        attachment->attachmentsArray.resize(MAX_FRAME);
        attachment->inUse = true;
        for (auto& texId : attachment->attachmentsArray) {
            texId = getNextIndex<FrameWork::Texture>();
            auto tex = getByIndex<FrameWork::Texture>(texId);
            tex->inUse = true;
            vulkanDevice->createImage(&tex->image, VkExtent2D(width, height), 1, 1, numSample, depthFormat, VK_IMAGE_TILING_OPTIMAL,
                isSampled ? (VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT) : VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            CreateImageView(tex->image, tex->imageView, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D);
            tex->sampler = CreateSampler(1); //为了后续使用
        }
    }
    else if (attachmentType == AttachmentType::Color) {
        attachment->attachmentsArray.resize(MAX_FRAME);
        attachment->inUse = true;
        for (auto& texId : attachment->attachmentsArray) {
            texId = getNextIndex<FrameWork::Texture>();
            auto tex = getByIndex<FrameWork::Texture>(texId);
            tex->inUse = true;
            vulkanDevice->createImage(&tex->image, VkExtent2D(width, height), 1, 1, numSample, swapChain.colorFormat, VK_IMAGE_TILING_OPTIMAL,
               isSampled ? (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT) : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            CreateImageView(tex->image, tex->imageView, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D);
            tex->sampler = CreateSampler(1);
        }
    }
}

void vulkanFrameWork::ReCreateAttachment() {
    for (int attachmentID = 0; attachmentID < attachmentBuffers.size(); attachmentID++) {
        auto attachment = getByIndex<FrameWork::VulkanAttachment>(attachmentID);
        // std::cerr << "attachment :" << attachmentID << "         rebuild !" << std::endl;
        if (attachment->isFollowWindow == false) {
            continue;
        }
        auto attachmentType = attachment->type;
        if (attachmentType == AttachmentType::Depth) {
            attachment->attachmentsArray.resize(MAX_FRAME);
            attachment->inUse = true;
            for (auto& texId : attachment->attachmentsArray) {
                destroyByIndex<FrameWork::Texture>(texId);
                auto tex = getByIndex<FrameWork::Texture>(texId);
                tex->inUse = true;
                vulkanDevice->createImage(&tex->image, VkExtent2D(windowWidth, windowHeight), 1, 1, attachment->samples, depthFormat, VK_IMAGE_TILING_OPTIMAL,
                    attachment->isSampled ? (VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT) : VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                CreateImageView(tex->image, tex->imageView, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D);
                tex->sampler = CreateSampler(1); //为了后续使用
            }
        }
        else if (attachmentType == AttachmentType::Color) {
            attachment->attachmentsArray.resize(MAX_FRAME);
            attachment->inUse = true;
            for (auto& texId : attachment->attachmentsArray) {
                destroyByIndex<FrameWork::Texture>(texId);
                auto tex = getByIndex<FrameWork::Texture>(texId);
                tex->inUse = true;
                vulkanDevice->createImage(&tex->image, VkExtent2D(windowWidth, windowHeight), 1, 1, attachment->samples, swapChain.colorFormat, VK_IMAGE_TILING_OPTIMAL,
                   attachment->isSampled ? (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT) : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                CreateImageView(tex->image, tex->imageView, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D);
                tex->sampler = CreateSampler(1);
            }
        }
    }
}

void vulkanFrameWork::CreateFrameBuffer(uint32_t &frameBufferId, const std::vector<uint32_t> &attachments, uint32_t width, uint32_t height, VkRenderPass renderPass) {
    //只允许传入颜色附件和深度附件
    for (auto& attachment : attachments) {
        auto attachmentObj = getByIndex<FrameWork::VulkanAttachment>(attachment);
        if (attachmentObj->type == AttachmentType::Present) {
            std::cerr << "you should use CreatePresentFrameBuffer to create present frame buffer object !" << std::endl;
            exit(-1);
        }
    }
    frameBufferId = getNextIndex<FrameWork::VulkanFBO>();
    auto fbo = getByIndex<FrameWork::VulkanFBO>(frameBufferId);
    fbo->isPresent = false;
    fbo->inUse = true;
    fbo->framebuffers.resize(MAX_FRAME);
    fbo->AttachmentsIdx = attachments;
    fbo->renderPass = renderPass;
    fbo->width = width;
    fbo->height = height;
    if (windowHeight == width && windowWidth == height) {
        fbo->isFollowWindow = true;
    }
    for (uint32_t i = 0; i < MAX_FRAME; i++) {
        std::vector<VkImageView> attachmentViews(attachments.size());
        for (uint32_t j = 0; j < attachments.size(); j++) {
            attachmentViews[j] = getByIndex<FrameWork::Texture>(getByIndex<FrameWork::VulkanAttachment>(attachments[j])->attachmentsArray[i])->imageView;
        }
        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = attachments.size();
        framebufferInfo.pAttachments = attachmentViews.data();
        framebufferInfo.width = getByIndex<FrameWork::VulkanAttachment>(attachments[0])->width;//附件的宽度默认是对齐的
        framebufferInfo.height = getByIndex<FrameWork::VulkanAttachment>(attachments[0])->height;//附件的高度默认是对齐的
        framebufferInfo.layers = 1;
        VK_CHECK_RESULT(vkCreateFramebuffer(device, &framebufferInfo, nullptr,&fbo->framebuffers[i]));
    }
}

void vulkanFrameWork::CreatePresentFrameBuffer(std::unique_ptr<FrameWork::VulkanFBO>& presentFBO, VkRenderPass renderPass) {
    //存储renderPass，方便重建
    if (presentFBO == nullptr) {
        presentFBO = std::make_unique<FrameWork::VulkanFBO>();
        presentFBO->inUse = true;
    }
    presentFBO->renderPass = renderPass;
    presentFBO->framebuffers.resize(swapChain.imageViews.size());
    for (uint32_t i = 0; i < swapChain.images.size(); i++) {
        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.width = windowWidth;
        framebufferInfo.height = windowHeight;
        framebufferInfo.layers = 1;
        framebufferInfo.pAttachments = &swapChain.imageViews[i];
        VK_CHECK_RESULT(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &presentFBO->framebuffers[i]));
    }
}

void vulkanFrameWork::RecreateFrameBuffer(uint32_t &frameBufferId) {
    auto fbo = getByIndex<FrameWork::VulkanFBO>(frameBufferId);
    fbo->inUse = true;


    // 只销毁framebuffer对象
    for (auto& framebuffer : fbo->framebuffers) {
        if (framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
    }
    auto size = fbo->framebuffers.size();
    fbo->framebuffers.clear();
    fbo->framebuffers.resize(size);
    std::cerr << "frameBuffer :" << frameBufferId << "         rebuild !" << std::endl;

    // 重建framebuffer
    for (uint32_t i = 0; i < MAX_FRAME; i++) {
        std::vector<VkImageView> attachments;
        for (auto& attachment : fbo->AttachmentsIdx) {
            auto imageView = textures[getByIndex<FrameWork::VulkanAttachment>(attachment)->attachmentsArray[i]]->imageView;
            if (imageView == VK_NULL_HANDLE) {
                std::cerr << "the imageView is Null Handle here" << std::endl;
                exit(-1);
            }
            attachments.push_back(imageView);
        }
        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = fbo->renderPass;
        framebufferInfo.attachmentCount = attachments.size();
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = windowWidth;
        framebufferInfo.height = windowHeight;
        framebufferInfo.layers = 1;
        VK_CHECK_RESULT(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &fbo->framebuffers[i]));
    }
}


void vulkanFrameWork::RecreatePresentFrameBuffer(std::unique_ptr<FrameWork::VulkanFBO>& presentFBO) {
    for (auto& framebuffer : presentFBO->framebuffers) {
        if (framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
    }
    CreatePresentFrameBuffer(presentFBO, presentFBO->renderPass);
}

void vulkanFrameWork::RegisterRenderPass(VkRenderPass renderPass, const std::string &name) {
    if (renderPasses.find(name) != renderPasses.end()) {
        std::cerr << "warning ! the" << name << " is already registered!" << std::endl;
    }
    renderPasses[name] = renderPass; //使用哈希
}


void vulkanFrameWork::UnRegisterRenderPass(const std::string &name) {
    if (renderPasses.find(name) != renderPasses.end()) {
        renderPasses.erase(name);
    }else {
        std::cerr << "Render pass " << name << " not found !" << std::endl;
        throw std::runtime_error("Render pass " + name + " not found!");
    }
}

void vulkanFrameWork::InitPipelineInfo(uint32_t &pipelineInfoIdx) {
    pipelineInfoIdx = getNextIndex<FrameWork::VulkanPipelineInfo>();
    auto pipelineInfo = getByIndex<FrameWork::VulkanPipelineInfo>(pipelineInfoIdx);
    pipelineInfo->inUse = true;
}



void vulkanFrameWork::AddPipelineVertexBindingDescription(uint32_t& pipelineInfoIdx,
    VkVertexInputBindingDescription &bindingDescription) {
    auto pipelineInfo = getByIndex<FrameWork::VulkanPipelineInfo>(pipelineInfoIdx);
    pipelineInfo->vertexBindingDescriptions.push_back(bindingDescription);
}

void vulkanFrameWork::AddPipelineVertexBindingDescription(uint32_t& pipelineInfoIdx,
    const std::vector<VkVertexInputBindingDescription> &bindingDescriptions) {
    auto pipelineInfo = getByIndex<FrameWork::VulkanPipelineInfo>(pipelineInfoIdx);
    pipelineInfo->vertexBindingDescriptions.insert(pipelineInfo->vertexBindingDescriptions.end(),
        bindingDescriptions.begin(),bindingDescriptions.end());
}

void vulkanFrameWork::AddPipelineVertexAttributeDescription(uint32_t& pipelineInfoIdx,
    const std::vector<VkVertexInputAttributeDescription> &attributeDescriptions) {
    auto pipelineInfo = getByIndex<FrameWork::VulkanPipelineInfo>(pipelineInfoIdx);
    pipelineInfo->vertexAttributeDescriptions.insert(pipelineInfo->vertexAttributeDescriptions.end(),
        attributeDescriptions.begin(),attributeDescriptions.end());
}

void vulkanFrameWork::SetPipelineInputAssembly(uint32_t& pipelineInfoIdx, VkPipelineInputAssemblyStateCreateInfo info) {
    auto pipelineInfo = getByIndex<FrameWork::VulkanPipelineInfo>(pipelineInfoIdx);
    pipelineInfo->inputAssembly = info;
}

void vulkanFrameWork::SetPipelineViewPort(uint32_t& pipelineInfoIdx, const VkViewport &viewport) {
    auto pipelineInfo = getByIndex<FrameWork::VulkanPipelineInfo>(pipelineInfoIdx);
    pipelineInfo->viewport = viewport;
}

void vulkanFrameWork::SetPipelineScissor(uint32_t& pipelineInfoIdx, const VkRect2D &scissor) {
    auto pipelineInfo = getByIndex<FrameWork::VulkanPipelineInfo>(pipelineInfoIdx);
    pipelineInfo->scissor = scissor;
}

void vulkanFrameWork::SetPipelineRasterizationState(uint32_t& pipelineInfoIdx,
    VkPipelineRasterizationStateCreateInfo createInfo) {
    auto pipelineInfo = getByIndex<FrameWork::VulkanPipelineInfo>(pipelineInfoIdx);
    pipelineInfo->rasterizationState = createInfo;
}

void vulkanFrameWork::SetPipelineMultiSampleState(uint32_t& pipelineInfoIdx, VkPipelineMultisampleStateCreateInfo info) {
    auto pipelineInfo = getByIndex<FrameWork::VulkanPipelineInfo>(pipelineInfoIdx);
    pipelineInfo->multisampleState = info;
}

void vulkanFrameWork::
SetPipelineDepthStencilState(uint32_t& pipelineInfoIdx, VkPipelineDepthStencilStateCreateInfo info) {
    auto pipelineInfo = getByIndex<FrameWork::VulkanPipelineInfo>(pipelineInfoIdx);
    pipelineInfo->depthStencilState = info;
}

void vulkanFrameWork::AddPipelineColorBlendState(uint32_t& pipelineInfoIdx, bool hasColor, BlendOp blendOp,
    VkColorComponentFlags colorWriteMask) {
    auto pipelineInfo = getByIndex<FrameWork::VulkanPipelineInfo>(pipelineInfoIdx);
    if (hasColor) {
        VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
       switch (blendOp) {
           case BlendOp::Transparent:
               colorBlendAttachment = {
               .blendEnable = VK_TRUE,
               .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
               .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
               .colorBlendOp = VK_BLEND_OP_ADD,
               .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
               .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
               .alphaBlendOp = VK_BLEND_OP_ADD,
               .colorWriteMask = colorWriteMask,
           };
               pipelineInfo->colorBlendAttachmentStates.push_back(colorBlendAttachment);
               break;
           case BlendOp::Opaque:
               colorBlendAttachment = {
               .blendEnable = VK_FALSE,
               .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
               .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
               .colorBlendOp = VK_BLEND_OP_ADD,
               .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
               .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
               .alphaBlendOp = VK_BLEND_OP_ADD,
               .colorWriteMask = colorWriteMask,
           };
               pipelineInfo->colorBlendAttachmentStates.push_back(colorBlendAttachment);
               break;
            case BlendOp::Multiply:
               colorBlendAttachment = {
               .blendEnable = VK_TRUE,
               .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
               .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
               .colorBlendOp = VK_BLEND_OP_MULTIPLY_EXT,
               .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
               .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
               .alphaBlendOp = VK_BLEND_OP_ADD,
               .colorWriteMask = colorWriteMask,
           };
               pipelineInfo->colorBlendAttachmentStates.push_back(colorBlendAttachment);
               break;
       }
    }else {
        VkPipelineColorBlendAttachmentState colorBlendAttachment = {
            .blendEnable = VK_FALSE,
            .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .colorBlendOp = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
            .alphaBlendOp = VK_BLEND_OP_ADD,
            .colorWriteMask = 0,
        };
        pipelineInfo->colorBlendAttachmentStates.push_back(colorBlendAttachment);
    }
}

VkPipelineColorBlendAttachmentState vulkanFrameWork::SetPipelineColorBlendAttachment(
    const FrameWork::ShaderInfo &shaderInfo) {
    return {
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = FrameWork::ShaderParse::blendFactorToVulkanBlendFactor[shaderInfo.shaderState.srcBlendFactor],
        .dstColorBlendFactor = FrameWork::ShaderParse::blendFactorToVulkanBlendFactor[shaderInfo.shaderState.dstBlendFactor],
        .colorBlendOp = FrameWork::ShaderParse::blendOpToVulkanBlendOp[shaderInfo.shaderState.blendOp],
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE, //这里不混合alpha
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        //保证所有颜色都写入,等到需要操作通道的时候在扩展shaderState
    };
}

std::vector<VkPipelineShaderStageCreateInfo> vulkanFrameWork::SetPipelineShaderStageInfo(
    const FrameWork::ShaderModulePackages &shaderModules) {
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    shaderStages.reserve(shaderModules.size());
    for (auto& shaderModule : shaderModules) {
        VkPipelineShaderStageCreateInfo shaderStage = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = shaderModule.first,
            .module = shaderModule.second,
            .pName = "main",
            //不需要常量特化,直接写在着色器里吧
        };
        shaderStages.emplace_back(shaderStage);
    }
    return shaderStages;
}

VkPipelineInputAssemblyStateCreateInfo vulkanFrameWork::SetPipelineInputAssembly() {
    return {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE, //片元启动一般不会用
    };
}

VkPipelineRasterizationStateCreateInfo vulkanFrameWork::SetRasterization(const FrameWork::ShaderInfo &shaderInfo) {
    return {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE, //不丢弃结果
        .polygonMode = FrameWork::ShaderParse::polygonModeToVulkanPolygonMode[shaderInfo.shaderState.polygonMode], //填充线段
        .cullMode = FrameWork::ShaderParse::cullModeToVulkanCullMode[shaderInfo.shaderState.faceCullOp],
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE, //遵循OpenGL的默认
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f, //不适用深度偏移
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f
    };
}

VkPipelineDepthStencilStateCreateInfo vulkanFrameWork::SetDepthStencil(const FrameWork::ShaderInfo &shaderInfo) {
    return {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = shaderInfo.shaderState.depthWrite ? VK_TRUE : VK_FALSE,
        .depthCompareOp = FrameWork::ShaderParse::compareModeToVulkanCompareMode[shaderInfo.shaderState.depthCompareOp],
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE, //模板等到使用时再拓展
        .front = {},
        .back = {},
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f
    };
}

void vulkanFrameWork::CreateVulkanPipeline(uint32_t& pipelineIdx, const std::string &name, uint32_t& pipelineInfoIdx,
                                           const std::string &renderPassName, uint32_t subpassIndex, const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts, uint32_t uniformNum,uint32_t texNum) {
    pipelineIdx = getNextIndex<FrameWork::VulkanPipeline>();
    auto vulkanPipeline = getByIndex<FrameWork::VulkanPipeline>(pipelineIdx);
    vulkanPipeline->inUse = true;
    auto vulkanPipelineInfo = getByIndex<FrameWork::VulkanPipelineInfo>(pipelineInfoIdx);
    vulkanPipelineInfo->inUse = true;
    vulkanPipeline->descriptorSetLayouts = descriptorSetLayouts;
    std::vector<VkDescriptorSetLayout> temp;
    if (descriptorSetLayouts.size() > 2) {
        std::cerr << "this pipeline only support two descriptorSetLayout this uniform and tex !!" << std::endl;
    }
    for (int i = 0; i < uniformNum; i++) {
        temp.push_back(descriptorSetLayouts[0]);
    }
    for (uint32_t i = 0; i < texNum; i++) {
        temp.push_back(descriptorSetLayouts.back());
    }
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .pNext = nullptr,
    .setLayoutCount = (uint32_t)(!temp.empty() ? temp.size() : 0),
    .pSetLayouts = (!temp.empty() ? temp.data() : nullptr),
    .pushConstantRangeCount = 0,
    .pPushConstantRanges = nullptr,
    };
    //先默认不使用常量推送
    VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &vulkanPipeline->pipelineLayout));

    std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos;

    for (auto& stage : vulkanPipelineInfo->shaderModules) {
        shaderStageCreateInfos.push_back(stage.second);
    }

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .vertexBindingDescriptionCount = static_cast<uint32_t>(vulkanPipelineInfo->vertexBindingDescriptions.size()),
            .pVertexBindingDescriptions = vulkanPipelineInfo->vertexBindingDescriptions.empty() ? nullptr : vulkanPipelineInfo->vertexBindingDescriptions.data(),
            .vertexAttributeDescriptionCount = static_cast<uint32_t>(vulkanPipelineInfo->vertexAttributeDescriptions.size()),
            .pVertexAttributeDescriptions = vulkanPipelineInfo->vertexAttributeDescriptions.empty() ? nullptr : vulkanPipelineInfo->vertexAttributeDescriptions.data(),
        };

    // 定义需要动态设置的状态
    std::vector dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    vulkanPipelineInfo->viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .viewportCount = 1, // 先不支持多视口渲染
        .pViewports = &vulkanPipelineInfo->viewport,
        .scissorCount = 1,
        .pScissors = &vulkanPipelineInfo->scissor,
    };

    VkPipelineColorBlendStateCreateInfo colorBlending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = static_cast<uint32_t>(vulkanPipelineInfo->colorBlendAttachmentStates.size()),
        .pAttachments = vulkanPipelineInfo->colorBlendAttachmentStates.data(),
    };

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stageCount = (uint32_t)shaderStageCreateInfos.size(),
        .pStages = shaderStageCreateInfos.data(),
        .pVertexInputState = &vertexInputInfo ,
        .pInputAssemblyState = &vulkanPipelineInfo->inputAssembly,
        .pTessellationState = nullptr,
        .pViewportState = &vulkanPipelineInfo->viewportState,
        .pRasterizationState = &vulkanPipelineInfo->rasterizationState,
        .pMultisampleState = &vulkanPipelineInfo->multisampleState,
        .pDepthStencilState = &vulkanPipelineInfo->depthStencilState,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = vulkanPipeline->pipelineLayout,
        .renderPass = renderPasses[renderPassName],
        .subpass = subpassIndex,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };

    VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &vulkanPipeline->pipeline));
}

void vulkanFrameWork::CreateVulkanPipeline(uint32_t &pipelineIdx, const std::string &name, uint32_t &pipelineInfoIdx,
    const std::string &renderPassName, uint32_t subpassIndex,
    const std::vector<VkDescriptorSetLayout> &descriptorSetLayout) {
    pipelineIdx = getNextIndex<FrameWork::VulkanPipeline>();
    auto vulkanPipeline = getByIndex<FrameWork::VulkanPipeline>(pipelineIdx);
    vulkanPipeline->inUse = true;
    auto vulkanPipelineInfo = getByIndex<FrameWork::VulkanPipelineInfo>(pipelineInfoIdx);
    vulkanPipelineInfo->inUse = true;
    vulkanPipeline->descriptorSetLayouts = descriptorSetLayout;
    std::vector<VkDescriptorSetLayout> temp;
    temp = descriptorSetLayout;
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .pNext = nullptr,
    .setLayoutCount = (uint32_t)(!temp.empty() ? temp.size() : 0),
    .pSetLayouts = (!temp.empty() ? temp.data() : nullptr),
    .pushConstantRangeCount = 0,
    .pPushConstantRanges = nullptr,
    };
    //先默认不使用常量推送
    VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &vulkanPipeline->pipelineLayout));

    std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos;

    for (auto& stage : vulkanPipelineInfo->shaderModules) {
        shaderStageCreateInfos.push_back(stage.second);
    }

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .vertexBindingDescriptionCount = static_cast<uint32_t>(vulkanPipelineInfo->vertexBindingDescriptions.size()),
            .pVertexBindingDescriptions = vulkanPipelineInfo->vertexBindingDescriptions.empty() ? nullptr : vulkanPipelineInfo->vertexBindingDescriptions.data(),
            .vertexAttributeDescriptionCount = static_cast<uint32_t>(vulkanPipelineInfo->vertexAttributeDescriptions.size()),
            .pVertexAttributeDescriptions = vulkanPipelineInfo->vertexAttributeDescriptions.empty() ? nullptr : vulkanPipelineInfo->vertexAttributeDescriptions.data(),
        };

    // 定义需要动态设置的状态
    std::vector dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    vulkanPipelineInfo->viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .viewportCount = 1, // 先不支持多视口渲染
        .pViewports = &vulkanPipelineInfo->viewport,
        .scissorCount = 1,
        .pScissors = &vulkanPipelineInfo->scissor,
    };

    VkPipelineColorBlendStateCreateInfo colorBlending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = static_cast<uint32_t>(vulkanPipelineInfo->colorBlendAttachmentStates.size()),
        .pAttachments = vulkanPipelineInfo->colorBlendAttachmentStates.data(),
    };



    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stageCount = (uint32_t)shaderStageCreateInfos.size(),
        .pStages = shaderStageCreateInfos.data(),
        .pVertexInputState = &vertexInputInfo ,
        .pInputAssemblyState = &vulkanPipelineInfo->inputAssembly,
        .pTessellationState = nullptr,
        .pViewportState = &vulkanPipelineInfo->viewportState,
        .pRasterizationState = &vulkanPipelineInfo->rasterizationState,
        .pMultisampleState = &vulkanPipelineInfo->multisampleState,
        .pDepthStencilState = &vulkanPipelineInfo->depthStencilState,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = vulkanPipeline->pipelineLayout,
        .renderPass = renderPasses[renderPassName],
        .subpass = subpassIndex,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };

    VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &vulkanPipeline->pipeline));
}

FrameWork::ShaderInfo vulkanFrameWork::CreateVulkanPipeline(uint32_t &pipelineIdx, const std::string &shaderPath,
    RenderPassType renderPassType, uint32_t subpass, uint32_t width, uint32_t height) {
    //获取到ShaderModules
    FrameWork::ShaderInfo shaderInfo = {};
    auto shaderModulePackages = resourceManager.GetShaderCaIShaderModule(device, shaderPath, shaderInfo);
    if ((shaderInfo.shaderTypeFlags & ShaderType::Comp) == ShaderType::Comp) {
        LOG_ERROR("The CreateVulkanPipeline Func can't create computer Shader Pipeline !");
    }
    //获取VulkanPipeline容器
    pipelineIdx = getNextIndex<FrameWork::VulkanPipeline>();
    auto pipeline  = getByIndex<FrameWork::VulkanPipeline>(pipelineIdx);
    //声明使用
    pipeline->inUse = true;

    //创建对应的VkDescriptorSetLayout
    std::vector<VkDescriptorSetLayoutBinding> descriptorBindings = {};
    if (! shaderInfo.vertProperties.baseProperties.empty()) {
        //因为这里的策略是所有的uniformObject结构体，所以只需要绑定第一个base值即可
        VkDescriptorSetLayoutBinding binding = {
            .binding = shaderInfo.vertProperties.baseProperties[0].binding,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .pImmutableSamplers = nullptr
        };
        descriptorBindings.push_back(binding);
    }
    if (! shaderInfo.fragProperties.baseProperties.empty()) {
        VkDescriptorSetLayoutBinding binding = {
            .binding = shaderInfo.fragProperties.baseProperties[0].binding,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr
        };
        descriptorBindings.push_back(binding);
    }

    //处理纹理绑定点
    if (! shaderInfo.vertProperties.textureProperties.empty()) {
        for (auto& texProperty : shaderInfo.vertProperties.textureProperties) {
            VkDescriptorSetLayoutBinding binding = {
                .binding = texProperty.binding,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                .pImmutableSamplers = nullptr
            };
            descriptorBindings.push_back(binding);
        }
    }

    if (! shaderInfo.fragProperties.textureProperties.empty()) {
        for (auto& texProperty : shaderInfo.fragProperties.textureProperties) {
            VkDescriptorSetLayoutBinding binding = {
                .binding = texProperty.binding,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = nullptr
            };
            descriptorBindings.push_back(binding);
        }
    }

    //创建SetLayout
    VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = (uint32_t)descriptorBindings.size(),
        .pBindings = descriptorBindings.data()
    };
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    VK_CHECK_RESULT(
        vkCreateDescriptorSetLayout(device, &setLayoutCreateInfo, nullptr, &layout)
        );
    pipeline->descriptorSetLayouts.push_back(layout);

    //创建管线
    //layout
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = (uint32_t)pipeline->descriptorSetLayouts.size(),
        .pSetLayouts = pipeline->descriptorSetLayouts.data(),
        .pushConstantRangeCount = 0,
        .pPushConstantRanges =  nullptr,
    };
    VK_CHECK_RESULT(
        vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo,
            nullptr, &pipeline->pipelineLayout)
        );

    //根据ShaderState创建管线
    auto shaderModuleInfos = SetPipelineShaderStageInfo(shaderModulePackages);
    auto colorBlendState = SetPipelineColorBlendAttachment(shaderInfo);
    auto depthStencilState = SetDepthStencil(shaderInfo);
    auto inputAssembly = SetPipelineInputAssembly();
    auto rasterization = SetRasterization(shaderInfo);

    auto bindingDescription = FrameWork::Vertex::getBindingDescription();
    auto attributeDescriptions = FrameWork::Vertex::getAttributeDescription();

    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount = (uint32_t)(shaderInfo.shaderState.inputVertex ? 1 : 0),
        .pVertexBindingDescriptions = shaderInfo.shaderState.inputVertex ? &bindingDescription : nullptr,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(shaderInfo.shaderState.inputVertex ? attributeDescriptions.size() : 0),
        .pVertexAttributeDescriptions = shaderInfo.shaderState.inputVertex ? attributeDescriptions.data() : nullptr
    };

    VkViewport viewport = {
        .x = 0,
        .y = 0,
        .width = (float)width,
        .height = (float)height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    VkRect2D scissor = {
        .offset = VkOffset2D(0, 0),
        .extent = VkExtent2D((width == -1 ? windowWidth : width),
            (height == -1 ? windowHeight : height))
    };


    VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor
    };

    std::vector dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .dynamicStateCount = (uint32_t)dynamicStates.size(),
        .pDynamicStates = dynamicStates.data()
    };

    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .rasterizationSamples = (renderPassType != RenderPassType::MsaaForward) ? VK_SAMPLE_COUNT_1_BIT : msaaSamples,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 0,
    };

    std::vector blendAttachments((renderPassType ==  RenderPassType::GBuffer ? 3 : 1), colorBlendState);

    VkPipelineColorBlendStateCreateInfo colorBlending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = static_cast<uint32_t>(blendAttachments.size()),
        .pAttachments = blendAttachments.data(),
    };

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stageCount = static_cast<uint32_t>(shaderModuleInfos.size()),
        .pStages = shaderModuleInfos.data(),
        .pVertexInputState = &vertexInputStateCreateInfo,
        .pInputAssemblyState = &inputAssembly,
        .pTessellationState = nullptr,
        .pViewportState = &viewportStateCreateInfo,
        .pRasterizationState = &rasterization,
        .pMultisampleState = &multisampleStateCreateInfo,
        .pDepthStencilState = &depthStencilState,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = pipeline->pipelineLayout,
        .renderPass = renderPassTable[renderPassType],
        .subpass = subpass,
    };
    VK_CHECK_RESULT(
        vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline->pipeline)
        );
    for (auto& shaderModule : shaderModulePackages ){
        vkDestroyShaderModule(
            device, shaderModule.second, nullptr
            );
    }
    return shaderInfo;
}

FrameWork::ShaderInfo vulkanFrameWork::CreateVulkanPipeline(uint32_t &pipelineIdx, const std::string &shaderPath,
    VkRenderPass renderPass, uint32_t subpass, uint32_t width, uint32_t height) {
        //获取到ShaderModules
    FrameWork::ShaderInfo shaderInfo = {};
    auto shaderModulePackages = resourceManager.GetShaderCaIShaderModule(device, shaderPath, shaderInfo);
    if ((shaderInfo.shaderTypeFlags & ShaderType::Comp) == ShaderType::Comp) {
        LOG_ERROR("The CreateVulkanPipeline Func can't create computer Shader Pipeline !");
    }
    //获取VulkanPipeline容器
    pipelineIdx = getNextIndex<FrameWork::VulkanPipeline>();
    auto pipeline  = getByIndex<FrameWork::VulkanPipeline>(pipelineIdx);
    //声明使用
    pipeline->inUse = true;

    //创建对应的VkDescriptorSetLayout
    std::vector<VkDescriptorSetLayoutBinding> descriptorBindings = {};
    if (! shaderInfo.vertProperties.baseProperties.empty()) {
        //因为这里的策略是所有的uniformObject结构体，所以只需要绑定第一个base值即可
        VkDescriptorSetLayoutBinding binding = {
            .binding = shaderInfo.vertProperties.baseProperties[0].binding,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .pImmutableSamplers = nullptr
        };
        descriptorBindings.push_back(binding);
    }
    if (! shaderInfo.fragProperties.baseProperties.empty()) {
        VkDescriptorSetLayoutBinding binding = {
            .binding = shaderInfo.fragProperties.baseProperties[0].binding,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr
        };
        descriptorBindings.push_back(binding);
    }

    //处理纹理绑定点
    if (! shaderInfo.vertProperties.textureProperties.empty()) {
        for (auto& texProperty : shaderInfo.vertProperties.textureProperties) {
            VkDescriptorSetLayoutBinding binding = {
                .binding = texProperty.binding,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                .pImmutableSamplers = nullptr
            };
            descriptorBindings.push_back(binding);
        }
    }

    if (! shaderInfo.fragProperties.textureProperties.empty()) {
        for (auto& texProperty : shaderInfo.fragProperties.textureProperties) {
            VkDescriptorSetLayoutBinding binding = {
                .binding = texProperty.binding,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = nullptr
            };
            descriptorBindings.push_back(binding);
        }
    }

    //创建SetLayout
    VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = (uint32_t)descriptorBindings.size(),
        .pBindings = descriptorBindings.data()
    };
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    VK_CHECK_RESULT(
        vkCreateDescriptorSetLayout(device, &setLayoutCreateInfo, nullptr, &layout)
        );
    pipeline->descriptorSetLayouts.push_back(layout);

    //创建管线
    //layout
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = (uint32_t)pipeline->descriptorSetLayouts.size(),
        .pSetLayouts = pipeline->descriptorSetLayouts.data(),
        .pushConstantRangeCount = 0,
        .pPushConstantRanges =  nullptr,
    };
    VK_CHECK_RESULT(
        vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo,
            nullptr, &pipeline->pipelineLayout)
        );

    //根据ShaderState创建管线
    auto shaderModuleInfos = SetPipelineShaderStageInfo(shaderModulePackages);
    auto colorBlendState = SetPipelineColorBlendAttachment(shaderInfo);
    auto depthStencilState = SetDepthStencil(shaderInfo);
    auto inputAssembly = SetPipelineInputAssembly();
    auto rasterization = SetRasterization(shaderInfo);

    auto bindingDescription = FrameWork::Vertex::getBindingDescription();
    auto attributeDescriptions = FrameWork::Vertex::getAttributeDescription();

    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount = (uint32_t)(shaderInfo.shaderState.inputVertex ? 1 : 0),
        .pVertexBindingDescriptions = shaderInfo.shaderState.inputVertex ? &bindingDescription : nullptr,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(shaderInfo.shaderState.inputVertex ? attributeDescriptions.size() : 0),
        .pVertexAttributeDescriptions = shaderInfo.shaderState.inputVertex ? attributeDescriptions.data() : nullptr
    };

    VkViewport viewport = {
        .x = 0,
        .y = 0,
        .width = (float)width,
        .height = (float)height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    VkRect2D scissor = {
        .offset = VkOffset2D(0, 0),
        .extent = VkExtent2D((width == -1 ? windowWidth : width),
            (height == -1 ? windowHeight : height))
    };


    VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor
    };

    std::vector dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .dynamicStateCount = (uint32_t)dynamicStates.size(),
        .pDynamicStates = dynamicStates.data()
    };

    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .rasterizationSamples = (shaderInfo.shaderState.msaa) ? VK_SAMPLE_COUNT_1_BIT : msaaSamples,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 0,
    };

    std::vector blendAttachments(shaderInfo.shaderState.outputNums, colorBlendState);

    VkPipelineColorBlendStateCreateInfo colorBlending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = static_cast<uint32_t>(blendAttachments.size()),
        .pAttachments = blendAttachments.data(),
    };

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stageCount = static_cast<uint32_t>(shaderModuleInfos.size()),
        .pStages = shaderModuleInfos.data(),
        .pVertexInputState = &vertexInputStateCreateInfo,
        .pInputAssemblyState = &inputAssembly,
        .pTessellationState = nullptr,
        .pViewportState = &viewportStateCreateInfo,
        .pRasterizationState = &rasterization,
        .pMultisampleState = &multisampleStateCreateInfo,
        .pDepthStencilState = &depthStencilState,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = pipeline->pipelineLayout,
        .renderPass = renderPass,
        .subpass = subpass,
    };
    VK_CHECK_RESULT(
        vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline->pipeline)
        );
    for (auto& shaderModule : shaderModulePackages ){
        vkDestroyShaderModule(
            device, shaderModule.second, nullptr
            );
    }
    return shaderInfo;
}

FrameWork::ShaderInfo vulkanFrameWork::CreateVulkanPipeline(uint32_t &pipelineIdx, const std::string &shaderPath, VkFormat colorFormat,
     uint32_t width, uint32_t height) {
    if (colorFormat == VK_FORMAT_UNDEFINED) {
        colorFormat = swapChain.colorFormat;
    }

    FrameWork::ShaderInfo shaderInfo = {};
    auto shaderModulePackages = resourceManager.GetShaderCaIShaderModule(device, shaderPath, shaderInfo);
    if ((shaderInfo.shaderTypeFlags & ShaderType::Comp) == ShaderType::Comp) {
        LOG_ERROR("The CreateVulkanPipeline Func can't create computer Shader Pipeline !");
    }
    //获取VulkanPipeline容器
    pipelineIdx = getNextIndex<FrameWork::VulkanPipeline>();
    auto pipeline  = getByIndex<FrameWork::VulkanPipeline>(pipelineIdx);
    //声明使用
    pipeline->inUse = true;

    //创建对应的VkDescriptorSetLayout
    std::vector<VkDescriptorSetLayoutBinding> descriptorBindings = {};
    if (! shaderInfo.vertProperties.baseProperties.empty()) {
        //因为这里的策略是所有的uniformObject结构体，所以只需要绑定第一个base值即可
        VkDescriptorSetLayoutBinding binding = {
            .binding = shaderInfo.vertProperties.baseProperties[0].binding,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .pImmutableSamplers = nullptr
        };
        descriptorBindings.push_back(binding);
    }
    if (! shaderInfo.fragProperties.baseProperties.empty()) {
        VkDescriptorSetLayoutBinding binding = {
            .binding = shaderInfo.fragProperties.baseProperties[0].binding,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr
        };
        descriptorBindings.push_back(binding);
    }

    //处理纹理绑定点
    if (! shaderInfo.vertProperties.textureProperties.empty()) {
        for (auto& texProperty : shaderInfo.vertProperties.textureProperties) {
            VkDescriptorSetLayoutBinding binding = {
                .binding = texProperty.binding,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                .pImmutableSamplers = nullptr
            };
            descriptorBindings.push_back(binding);
        }
    }

    if (! shaderInfo.fragProperties.textureProperties.empty()) {
        for (auto& texProperty : shaderInfo.fragProperties.textureProperties) {
            VkDescriptorSetLayoutBinding binding = {
                .binding = texProperty.binding,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = nullptr
            };
            descriptorBindings.push_back(binding);
        }
    }

    //创建SetLayout
    VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = (uint32_t)descriptorBindings.size(),
        .pBindings = descriptorBindings.data()
    };
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    VK_CHECK_RESULT(
        vkCreateDescriptorSetLayout(device, &setLayoutCreateInfo, nullptr, &layout)
        );
    pipeline->descriptorSetLayouts.push_back(layout);

    //创建管线
    //layout
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = (uint32_t)pipeline->descriptorSetLayouts.size(),
        .pSetLayouts = pipeline->descriptorSetLayouts.data(),
        .pushConstantRangeCount = 0,
        .pPushConstantRanges =  nullptr,
    };
    VK_CHECK_RESULT(
        vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo,
            nullptr, &pipeline->pipelineLayout)
        );

    //根据ShaderState创建管线
    auto shaderModuleInfos = SetPipelineShaderStageInfo(shaderModulePackages);
    auto colorBlendState = SetPipelineColorBlendAttachment(shaderInfo);
    auto depthStencilState = SetDepthStencil(shaderInfo);
    auto inputAssembly = SetPipelineInputAssembly();
    auto rasterization = SetRasterization(shaderInfo);

    auto bindingDescription = FrameWork::Vertex::getBindingDescription();
    auto attributeDescriptions = FrameWork::Vertex::getAttributeDescription();

    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount = (uint32_t)(shaderInfo.shaderState.inputVertex ? 1 : 0),
        .pVertexBindingDescriptions = shaderInfo.shaderState.inputVertex ? &bindingDescription : nullptr,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(shaderInfo.shaderState.inputVertex ? attributeDescriptions.size() : 0),
        .pVertexAttributeDescriptions = shaderInfo.shaderState.inputVertex ? attributeDescriptions.data() : nullptr
    };

    VkViewport viewport = {
        .x = 0,
        .y = 0,
        .width = (float)width,
        .height = (float)height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    VkRect2D scissor = {
        .offset = VkOffset2D(0, 0),
        .extent = VkExtent2D((width == -1 ? windowWidth : width),
            (height == -1 ? windowHeight : height))
    };


    VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor
    };

    std::vector dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .dynamicStateCount = (uint32_t)dynamicStates.size(),
        .pDynamicStates = dynamicStates.data()
    };

    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .rasterizationSamples = (!shaderInfo.shaderState.msaa) ? VK_SAMPLE_COUNT_1_BIT : msaaSamples,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 0,
    };

    std::vector blendAttachments(shaderInfo.shaderState.outputNums, colorBlendState);

    VkPipelineColorBlendStateCreateInfo colorBlending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = static_cast<uint32_t>(blendAttachments.size()),
        .pAttachments = blendAttachments.data(),
    };

    std::vector colorFormats(shaderInfo.shaderState.outputNums, colorFormat);

    //Dynamic Rendering设置
    VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo{};
    pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineRenderingCreateInfo.colorAttachmentCount = static_cast<uint32_t>(colorFormats.size());
    pipelineRenderingCreateInfo.pColorAttachmentFormats = colorFormats.data();
    if (shaderInfo.shaderState.depthWrite)
        pipelineRenderingCreateInfo.depthAttachmentFormat = depthFormat;
    else
        pipelineRenderingCreateInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED;

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &pipelineRenderingCreateInfo,
        .flags = 0,
        .stageCount = static_cast<uint32_t>(shaderModuleInfos.size()),
        .pStages = shaderModuleInfos.data(),
        .pVertexInputState = &vertexInputStateCreateInfo,
        .pInputAssemblyState = &inputAssembly,
        .pTessellationState = nullptr,
        .pViewportState = &viewportStateCreateInfo,
        .pRasterizationState = &rasterization,
        .pMultisampleState = &multisampleStateCreateInfo,
        .pDepthStencilState = &depthStencilState,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = pipeline->pipelineLayout,
    };
    //启用dynamic Rendering不使用RenderPass
    VK_CHECK_RESULT(
        vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline->pipeline)
        );
    for (auto& shaderModule : shaderModulePackages ){
        vkDestroyShaderModule(
            device, shaderModule.second, nullptr
            );
    }
    return shaderInfo;
}

FrameWork::CompShaderInfo vulkanFrameWork::CreateCompPipeline(uint32_t &pipelineID, const std::string &shaderPath) {
    FrameWork::CompShaderInfo compShaderInfo;
    auto shaderModulePacks = resourceManager.GetCompShaderModule(device, shaderPath, compShaderInfo);
    //获取pipelineID
    pipelineID = getNextIndex<FrameWork::VulkanPipeline>();
    auto pipeline = getByIndex<FrameWork::VulkanPipeline>(pipelineID);
    pipeline->inUse = true;

    //绑定descriptorSetLayoutBinding
    std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;

    //绑定Uniform
    if (!compShaderInfo.shaderProperties.baseProperties.empty()) {
        VkDescriptorSetLayoutBinding binding = {
            .binding = compShaderInfo.shaderProperties.baseProperties[0].binding,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = nullptr
        };
        descriptorSetLayoutBindings.push_back(binding);
    }
    //Texture
    for (auto& texPro : compShaderInfo.shaderProperties.textureProperties) {
        VkDescriptorSetLayoutBinding binding = {
            .binding = texPro.binding,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = nullptr
        };
        descriptorSetLayoutBindings.push_back(binding);
    }

    //SSBO
    for (auto& ssbo : compShaderInfo.ssbos) {
        VkDescriptorSetLayoutBinding binding = {
            .binding = ssbo.binding,
            .descriptorType = FrameWork::ShaderParse::storageTypeToDescriptorType[ssbo.type],
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = nullptr
        };
        descriptorSetLayoutBindings.push_back(binding);
    }

    //创建SetLayout
    VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = (uint32_t)descriptorSetLayoutBindings.size(),
        .pBindings = descriptorSetLayoutBindings.data()
    };
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    VK_CHECK_RESULT(
        vkCreateDescriptorSetLayout(device, &setLayoutCreateInfo, nullptr, &layout)
        );
    pipeline->descriptorSetLayouts.push_back(layout);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = (uint32_t)pipeline->descriptorSetLayouts.size();
    pipelineLayoutInfo.pSetLayouts = pipeline->descriptorSetLayouts.data();

    VK_CHECK_RESULT(
        vkCreatePipelineLayout(device, &pipelineLayoutInfo,
            nullptr, &pipeline->pipelineLayout)
        );

    VkComputePipelineCreateInfo computePipelineCreateInfo{};
    computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineCreateInfo.layout = pipeline->pipelineLayout;
    computePipelineCreateInfo.stage = SetPipelineShaderStageInfo(shaderModulePacks).back(); //因为只有一个
    VK_CHECK_RESULT(
        vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &pipeline->pipeline)
        );

    for (auto& shaderModule : shaderModulePacks ){
        vkDestroyShaderModule(
            device, shaderModule.second, nullptr
            );
    }


    return compShaderInfo;
}


void vulkanFrameWork::BeginRenderPass(const std::string &renderPassName, uint32_t frameBufferID, uint32_t renderWidth, uint32_t renderHeight, VkClearColorValue clearColor) const {
    VkClearValue clearValues[2];
    clearValues[0].color = clearColor;
    clearValues[1].depthStencil = {1.0f, 0};
    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = vulkanRenderAPI.GetRenderPass(renderPassName);
    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.renderArea.extent.width = renderWidth;
    renderPassBeginInfo.renderArea.extent.height = renderHeight;
    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues = clearValues;
    renderPassBeginInfo.framebuffer = vulkanRenderAPI.getByIndex<FrameWork::VulkanFBO>(frameBufferID)->framebuffers[
        vulkanRenderAPI.GetCurrentFrame()];
    vkCmdBeginRenderPass(GetCurrentCommandBuffer(), &renderPassBeginInfo,
                         VK_SUBPASS_CONTENTS_INLINE);
    VkViewport viewport = {};
    viewport.width = (float) renderWidth;
    viewport.height = (float) renderHeight;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(GetCurrentCommandBuffer(), 0, 1, &viewport);
    VkRect2D scissor = {};
    scissor.extent.width = renderWidth;
    scissor.extent.height = renderHeight;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vkCmdSetScissor(GetCurrentCommandBuffer(), 0, 1, &scissor);
}

void vulkanFrameWork::BeginRenderPass(VkRenderPass renderPass, uint32_t frameBufferID, uint32_t renderWidth, uint32_t renderHeight) const {
    VkClearValue clearValues[2];
    clearValues[0].color = vulkanRenderAPI.defaultClearColor;
    clearValues[1].depthStencil = {1.0f, 0};
    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.renderArea.extent.width = renderWidth;
    renderPassBeginInfo.renderArea.extent.height = renderHeight;
    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues = clearValues;
    renderPassBeginInfo.framebuffer = vulkanRenderAPI.getByIndex<FrameWork::VulkanFBO>(frameBufferID)->framebuffers[
        vulkanRenderAPI.GetCurrentFrame()];
    vkCmdBeginRenderPass(GetCurrentCommandBuffer(), &renderPassBeginInfo,
                         VK_SUBPASS_CONTENTS_INLINE);
    VkViewport viewport = {};
    viewport.width = (float) renderWidth;
    viewport.height = (float) renderHeight;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(GetCurrentCommandBuffer(), 0, 1, &viewport);
    VkRect2D scissor = {};
    scissor.extent.width = renderWidth;
    scissor.extent.height = renderHeight;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vkCmdSetScissor(GetCurrentCommandBuffer(), 0, 1, &scissor);
}

void vulkanFrameWork::EndRenderPass() const {
    vkCmdEndRenderPass(GetCurrentCommandBuffer());
}

void vulkanFrameWork::EndCommandBuffer() const {
    VK_CHECK_RESULT(vkEndCommandBuffer(GetCurrentCommandBuffer()));
}

VkCommandBuffer vulkanFrameWork::BeginCommandBuffer() const {
    auto cmdBuffer = GetCurrentCommandBuffer();
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmdBuffer, &beginInfo);
    return cmdBuffer;
}



void vulkanFrameWork::CreateMaterial(uint32_t &materialIdx, const std::vector<FrameWork::TextureFullData>& texDatas) {
    materialIdx = getNextIndex<FrameWork::Material>();
    auto material = getByIndex<FrameWork::Material>(materialIdx);
    material->inUse = true;
    material->textures.resize(texDatas.size());
    for (uint32_t i = 0; i < texDatas.size(); i++) {
        CreateTexture(material->textures[i], texDatas[i]);
    }
}


void vulkanFrameWork::CreateMaterialData(FrameWork::CaIMaterial &caiMaterial) {
    auto shaderRef = caiMaterial.GetShader();
    caiMaterial.dataID = getNextIndex<FrameWork::MaterialData>();
    auto materialData = getByIndex<FrameWork::MaterialData>(caiMaterial.dataID);
    materialData->inUse = true;
    auto pipeline = getByIndex<FrameWork::VulkanPipeline>(shaderRef->GetPipelineID());
    auto shaderInfo = shaderRef->GetShaderInfo();


    if (!shaderInfo.vertProperties.baseProperties.empty()) {
        materialData->vertexUniformBuffers.resize(MAX_FRAME);
    }
    if (!shaderInfo.fragProperties.baseProperties.empty()) {
        materialData->fragmentUniformBuffers.resize(MAX_FRAME);
    }
    for (uint32_t i = 0; i < MAX_FRAME; i++) {
        if(!shaderInfo.vertProperties.baseProperties.empty()) {
            materialData->vertexUniformBuffers[i] =
                CreateUniformBuffer(shaderInfo.vertProperties.baseProperties);
        }
        if (!shaderInfo.fragProperties.baseProperties.empty()) {
            materialData->fragmentUniformBuffers[i] =
                CreateUniformBuffer(shaderInfo.fragProperties.baseProperties);
        }
    }

    //AllocateDescriptorSet
    materialData->descriptorSets.resize(MAX_FRAME);
    std::vector<VkWriteDescriptorSet> descriptorWrites;
    std::vector<VkDescriptorType> descriptorTypes;
    descriptorTypes.push_back(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    if (!shaderInfo.vertProperties.textureProperties.empty() || !shaderInfo.fragProperties.textureProperties.empty()) {
        descriptorTypes.push_back(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }
    for (uint32_t i = 0; i < MAX_FRAME; i++) {
        vulkanDescriptorPool.AllocateDescriptorSet(pipeline->descriptorSetLayouts.back(),
            descriptorTypes, materialData->descriptorSets[i]
            );

        if (!materialData->vertexUniformBuffers.empty()) {
            VkWriteDescriptorSet descriptorWrite = {
               .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
               .pNext = nullptr,
               .dstSet = materialData->descriptorSets[i],
               .dstBinding = shaderInfo.vertProperties.baseProperties[0].binding, //绑定点是首位
               .dstArrayElement = 0,
               .descriptorCount = 1,
               .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
               .pImageInfo = nullptr,
               .pBufferInfo = &materialData->vertexUniformBuffers[i].descriptor,
            };
            descriptorWrites.push_back(descriptorWrite);
        }

        if (!materialData->fragmentUniformBuffers.empty()) {
            VkWriteDescriptorSet descriptorWrite = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = materialData->descriptorSets[i],
                .dstBinding = shaderInfo.fragProperties.baseProperties[0].binding,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pImageInfo = nullptr,
                .pBufferInfo = &materialData->fragmentUniformBuffers[i].descriptor,
            };
            descriptorWrites.push_back(descriptorWrite);
        }

        vkUpdateDescriptorSets(device,
            static_cast<uint32_t>(descriptorWrites.size()),
            descriptorWrites.data(), 0, nullptr
            );

        //对于纹理，后续需要添加鲁棒性，可以使用一个默认纹理来防止验证层报错
    }

}

void vulkanFrameWork::CreateCompMaterialData(FrameWork::CompMaterial &compMaterial) {
    auto shaderRef = compMaterial.GetShader();
    compMaterial.compDataID = getNextIndex<FrameWork::CompMaterialData>();
    auto compData = getByIndex<FrameWork::CompMaterialData>(compMaterial.compDataID);
    compData->inUse = true;
    auto pipeline = getByIndex<FrameWork::VulkanPipeline>(shaderRef->GetPipelineID());
    auto shaderInfo = shaderRef->GetShaderInfo();


    //创建uniformBuffer
    if (!shaderInfo.shaderProperties.baseProperties.empty()) {
        compData->uniformBuffers.resize(MAX_FRAME);
    }

    for (int i = 0; i < MAX_FRAME; i++) {
        if (!shaderInfo.shaderProperties.baseProperties.empty()) {
            compData->uniformBuffers[i] =
                CreateUniformBuffer(shaderInfo.shaderProperties.baseProperties);
        }
    }

    //Allocate DescriptorSet
    compData->descriptorSets.resize(MAX_FRAME);
    std::vector<VkWriteDescriptorSet> descriptorWrites;
    std::vector<VkDescriptorType> descriptorTypes;

    if (!shaderInfo.shaderProperties.baseProperties.empty()) {
        descriptorTypes.push_back(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    }
    if (!shaderInfo.shaderProperties.textureProperties.empty()) {
        descriptorTypes.push_back(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }
    if (!shaderInfo.ssbos.empty()) {
        for (auto& ssbo : shaderInfo.ssbos) {
            if (ssbo.type == StorageObjectType::Buffer) {
                descriptorTypes.push_back(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
            }else {
                descriptorTypes.push_back(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
            }
        }
    }

    for (uint32_t i = 0; i < MAX_FRAME; i++) {
        vulkanDescriptorPool.AllocateDescriptorSet(pipeline->descriptorSetLayouts.back(),
            descriptorTypes, compData->descriptorSets[i]
        );

        if (!compData->uniformBuffers.empty()) {
            VkWriteDescriptorSet descriptorWrite = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = compData->descriptorSets[i],
                .dstBinding = shaderInfo.shaderProperties.baseProperties[0].binding, //绑定点是首位
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pImageInfo = nullptr,
                .pBufferInfo = &compData->uniformBuffers[i].descriptor,
             };
            descriptorWrites.push_back(descriptorWrite);
        }

        vkUpdateDescriptorSets(device,
        static_cast<uint32_t>(descriptorWrites.size()),
        descriptorWrites.data(), 0, nullptr
        );
    }

    //Texture 和 SSBO需要在后续Set绑定
}

void vulkanFrameWork::UpdateUniformBuffer(const std::vector<FrameWork::Buffer> &uniformBuffer, const std::vector<void *> &data,
                                          const std::vector<uint32_t>& sizes, uint32_t offset) {//这里
    for (int i = 0 ; i < uniformBuffer.size(); i++) {
        memcpy(static_cast<char*>(uniformBuffer[i].mapped) + offset, data[i], sizes[i]);
    }
    //这里的偏置对应的是飞行帧数
}

VkSampler vulkanFrameWork::CreateSampler(uint32_t mipmapLevels) {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;  // U轴
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;  // V轴
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;  // W轴
    // 开启各向异性filter
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = vulkanDevice->properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    // 这里填false，纹理采样坐标范围就是正常的[0, 1)，如果填true，就会变成[0, texWidth)和[0, texHeight)，绝大部分情况下都是用[0, 1)
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    // 一般用不到这个，某些场景，比如shadow map的percentage-closer filtering会用到
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    // mipmap设置
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(mipmapLevels);

    VkSampler sampler;
    if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
        throw std::runtime_error("failed to create texture sampler!");

    return sampler;
}

void vulkanFrameWork::SetUpStaticMesh(unsigned int &meshID, std::vector<FrameWork::Vertex> &vertices,
    std::vector<uint32_t> &indices, bool skinned) {
    meshID = getNextIndex<FrameWork::Mesh>();
    auto meshBuffer = getByIndex<FrameWork::Mesh>(meshID);
    meshBuffer->indexCount = indices.size();
    meshBuffer->vertexCount = vertices.size();

    //Vertex Buffer
    //TO do 光追的处理
    VkDeviceSize vertexBufferSize = sizeof(FrameWork::Vertex) * vertices.size();
    CreateGPUBuffer(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, meshBuffer->VertexBuffer, static_cast<void*>(vertices.data()));

    //光追的话需要获得GPU地址

    // Index  Buffer
    VkDeviceSize indexBufferSize = sizeof(uint32_t) * indices.size();
    CreateGPUBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, meshBuffer->IndexBuffer, indices.data());

    //非常重要设置inUse
    meshBuffer->inUse = true;
}


void vulkanFrameWork::LoadVulkanModel(uint32_t &modelDataID, const std::string &fileName, ModelType modelType,
    TextureTypeFlags textureTypeFlags, glm::vec3 position, float scale) {
    modelDataID = getNextIndex<FrameWork::VulkanModelData>();
    auto vulkanModelData = getByIndex<FrameWork::VulkanModelData>(modelDataID);
    FrameWork::ModelData modelData = {};
    modelData.meshDatas = resourceManager.LoadMesh(fileName, modelType, textureTypeFlags, scale);
    vulkanModelData->inUse = true;
    vulkanModelData->position = position;
    //两者大小对应保证模型材质正常
    vulkanModelData->meshIDs.resize(modelData.meshDatas.size());
    vulkanModelData->textures.resize(modelData.meshDatas.size());
    //构建包围盒子
    std::vector<FrameWork::AABB> triangleBoundingBoxes;
    bool firstTriangle = true;
    for (auto& m : modelData.meshDatas) {
        for (int i = 0; i < m.indices.size(); i += 3) {
            glm::vec3 v1 = m.vertices[m.indices[i]].position;
            glm::vec3 v2 = m.vertices[m.indices[i + 1]].position;
            glm::vec3 v3 = m.vertices[m.indices[i + 2]].position;

            FrameWork::AABB triangleAABB;
            triangleAABB.max = glm::max(glm::max(v1, v2), v3);
            triangleAABB.min = glm::min(glm::min(v1, v2), v3);

            if (firstTriangle) {
                vulkanModelData->aabb = triangleAABB;
                firstTriangle = false;
            } else {
                vulkanModelData->aabb.max = glm::max(vulkanModelData->aabb.max, triangleAABB.max);
                vulkanModelData->aabb.min = glm::min(vulkanModelData->aabb.min, triangleAABB.min);
            }

            triangleBoundingBoxes.push_back(std::move(triangleAABB));
        }
    }
    vulkanModelData->triangleBoundingBoxs = std::make_unique<std::vector<FrameWork::AABB>>(std::move(triangleBoundingBoxes));
    //构建包围盒

    for (int i = 0; i < modelData.meshDatas.size(); i++) {
        //创建顶点信息
        SetUpStaticMesh(vulkanModelData->meshIDs[i], modelData.meshDatas[i].vertices, modelData.meshDatas[i].indices, false);
        for (int j = 0; j < modelData.meshDatas[i].texData.size(); j++) {
            //创建对应Mesh对应的纹理,type是纹理类型
            auto type = modelData.meshDatas[i].texData[j].type;
            CreateTexture(vulkanModelData->textures[i][type] , modelData.meshDatas[i].texData[j]);
        }
    }
}

void vulkanFrameWork::BindMesh(VkCommandBuffer cmdBuffer, uint32_t meshData) {
    auto mesh = vulkanRenderAPI.getByIndex<FrameWork::Mesh>(meshData);
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &mesh->VertexBuffer.buffer, offsets);
    vkCmdBindIndexBuffer(cmdBuffer, mesh->IndexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cmdBuffer, mesh->indexCount, 1, 0, 0, 0); //没有进行实例渲染
}


void vulkanFrameWork::GenFaceData(uint32_t &modelDataID, const glm::vec3 &position, const glm::vec3 &normal, float width,
                                  float height, const std::string &texPath) {
    modelDataID = getNextIndex<FrameWork::VulkanModelData>();
    auto modelData = getByIndex<FrameWork::VulkanModelData>(modelDataID);
    modelData->inUse = true;

    glm::vec3 originNormal = {0, 0, 1};
    auto dot = glm::dot(glm::normalize(normal), originNormal);
    auto rotateMatrix = glm::mat4(1.0f);
    if (dot <= -0.9999) {
        rotateMatrix = glm::rotate(rotateMatrix, glm::radians(180.0f), {0, 1, 0});
    }else if (dot < 0.9999) {
        auto cross = glm::cross(originNormal, glm::normalize(normal));
        cross = glm::normalize(cross);
        rotateMatrix = glm::rotate(rotateMatrix, glm::acos(dot), cross);
    }

    std::vector<FrameWork::Vertex> vertices(4);
    // 计算平面四个角的位置（在XY平面上）
    float halfWidth = width * 0.5f;
    float halfHeight = height * 0.5f;

    // 左下角
    vertices[0].position =glm::vec3(-halfWidth, -halfHeight, 0.0f);
    vertices[0].normal = glm::vec3(0.0f, 0.0f, 1.0f);
    vertices[0].tangent = glm::vec3(1.0f, 0.0f, 0.0f);
    vertices[0].texCoord = glm::vec2(0.0f, 1.0f);

    // 右下角
    vertices[1].position = glm::vec3(halfWidth, -halfHeight, 0.0f);
    vertices[1].normal = glm::vec3(0.0f, 0.0f, 1.0f);
    vertices[1].tangent = glm::vec3(1.0f, 0.0f, 0.0f);
    vertices[1].texCoord = glm::vec2(1.0f, 1.0f);

    // 右上角
    vertices[2].position =glm::vec3(halfWidth, halfHeight, 0.0f);
    vertices[2].normal = glm::vec3(0.0f, 0.0f, 1.0f);
    vertices[2].tangent = glm::vec3(1.0f, 0.0f, 0.0f);
    vertices[2].texCoord = glm::vec2(1.0f, 0.0f);

    // 左上角
    vertices[3].position =glm::vec3(-halfWidth, halfHeight, 0.0f);
    vertices[3].normal = glm::vec3(0.0f, 0.0f, 1.0f);
    vertices[3].tangent = glm::vec3(1.0f, 0.0f, 0.0f);
    vertices[3].texCoord = glm::vec2(0.0f, 0.0f);

    modelData->meshIDs.resize(1);
    modelData->textures.resize(1);

    //提前进行旋转
    for (auto& v : vertices) {
        v.normal = glm::vec3(rotateMatrix * glm::vec4(v.normal, 1.0f));
        v.position = glm::vec3(rotateMatrix * glm::vec4(v.position, 1.0f));
        v.tangent = glm::vec3(rotateMatrix * glm::vec4(v.tangent, 1.0f));
    }
    modelData->position = position;

    std::vector<uint32_t> indices = {
        0, 1, 2,  // 第一个三角形
        2, 3, 0   // 第二个三角形
    };
    SetUpStaticMesh(modelData->meshIDs.back(), vertices, indices, false);

    //构建包围盒子
    std::vector<FrameWork::AABB> triangleBoundingBoxes;
    bool firstTriangle = true;
    for (int i = 0; i < indices.size(); i += 3) {
        glm::vec3 v1 = vertices[indices[i]].position;
        glm::vec3 v2 = vertices[indices[i + 1]].position;
        glm::vec3 v3 = vertices[indices[i + 2]].position;

        FrameWork::AABB triangleAABB;
        triangleAABB.max = glm::max(glm::max(v1, v2), v3);
        triangleAABB.min = glm::min(glm::min(v1, v2), v3);

        if (firstTriangle) {
            modelData->aabb = triangleAABB;
            firstTriangle = false;
        } else {
            modelData->aabb.max = glm::max(modelData->aabb.max, triangleAABB.max);
            modelData->aabb.min = glm::min(modelData->aabb.min, triangleAABB.min);
        }

        triangleBoundingBoxes.push_back(triangleAABB);
    }
    modelData->triangleBoundingBoxs = std::make_unique<std::vector<FrameWork::AABB>>(std::move(triangleBoundingBoxes));
    //构建包围盒

    if (!texPath.empty()) {
        auto textureData = resourceManager.LoadTextureFullData(texPath, DiffuseColor);
        CreateTexture(modelData->textures.back()[DiffuseColor], textureData);
    }
}






void vulkanFrameWork::CreateGPUStorgeBuffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, FrameWork::Buffer &buffer, void *data) {
    FrameWork::Buffer stagingBuffer;
    vulkanDevice->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &stagingBuffer, size, data);
    vulkanDevice->createBuffer(usageFlags | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &buffer, size, nullptr);
    vulkanDevice->copyBuffer(&stagingBuffer, &buffer, graphicsQueue);

    //删除中转内存
    stagingBuffer.destroy();
}

void vulkanFrameWork::CreateHostVisibleStorageBuffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, FrameWork::Buffer &buffer,
    void *data) {
    vulkanDevice->createBuffer(usageFlags | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &buffer, size, data);
}

vulkanFrameWork& vulkanFrameWork::GetInstance() {
    static vulkanFrameWork vulkan;
    return vulkan;
}


uint32_t vulkanFrameWork::GetFrameWidth() const {
    return windowWidth;
}

uint32_t vulkanFrameWork::GetFrameHeight() const {
    return windowHeight;
}

uint32_t vulkanFrameWork::GetCurrentFrame() const {
    return currentFrame;
}

VkRenderPass vulkanFrameWork::GetRenderPass(const std::string &name) const {
    if (renderPasses.find(name) == renderPasses.end()) {
        return VK_NULL_HANDLE;
    }else {
        return renderPasses.at(name);
    }
}


FrameWork::VulkanDevice* vulkanFrameWork::GetVulkanDevice() const {
    return vulkanDevice;
}

void vulkanFrameWork::SetTitle(const std::string &title) {
    this->title = title;
}

VkCommandBuffer vulkanFrameWork::GetCurrentCommandBuffer() const {
    return drawCmdBuffers[currentFrame];
}

uint32_t vulkanFrameWork::GetCurrentImageIndex() const {
    return imageIndex;
}

const VulkanSwapChain & vulkanFrameWork::GetVulkanSwapChain() const {
    return swapChain;
}

VkInstance& vulkanFrameWork::GetVulkanInstance() {
    return instance;
}

VkQueue vulkanFrameWork::GetVulkanGraphicsQueue() const {
    return graphicsQueue;
}

VkPhysicalDevice vulkanFrameWork::GetVulkanPhysicalDevice() const {
    return physicalDevice;
}

VkFormat vulkanFrameWork::GetDepthFormat() const {
    return depthFormat;
}

VkSampleCountFlagBits vulkanFrameWork::GetSampleCount() const {
    return msaaSamples;
}

void vulkanFrameWork::DeleteTexture(uint32_t id) {
    std::lock_guard<std::mutex> lock(texDeleteMutex);
    if (id < textures.size() && textures[id]->inUse) {
        textureReleaseQueue.emplace_back(id, MAX_FRAME + 1);
    }
}

void vulkanFrameWork::DeleteMesh(uint32_t id) {
    std::lock_guard<std::mutex> lock(meshDeleteMutex);
    if (id < meshes.size() && meshes[id]->inUse) {
        meshReleaseQueue.emplace_back(id, MAX_FRAME + 1);
    }
}

void vulkanFrameWork::DeleteAttachment(uint32_t id) {
    std::lock_guard<std::mutex> lock(attachmentDeleteMutex);
    if (id < attachmentBuffers.size() && attachmentBuffers[id]->inUse) {
        attachmentReleaseQueue.emplace_back(id, MAX_FRAME + 1);
    }
}

void vulkanFrameWork::DeleteFBO(uint32_t id) {
    std::lock_guard<std::mutex> lock(fboDeleteMutex);
    if (id < vulkanFBOs.size() && vulkanFBOs[id]->inUse) {
        fboReleaseQueue.emplace_back(id, MAX_FRAME + 1);
    }
}

void vulkanFrameWork::DeletePipeline(uint32_t id) {
    std::lock_guard<std::mutex> lock(pipelineDeleteMutex);
    if (id < vulkanPipelines.size() && vulkanPipelines[id]->inUse) {
        pipelineReleaseQueue.emplace_back(id, MAX_FRAME + 1);
    }
}

void vulkanFrameWork::DeleteMaterialData(uint32_t id) {
    std::lock_guard<std::mutex> lock(materialDeleteMutex);
    if (id < materialDatas_.size() && materialDatas_[id]->inUse) {
        materialDataReleaseQueue.emplace_back(id, MAX_FRAME + 1);
    }
}

void vulkanFrameWork::DeleteModelData(uint32_t id) {
    std::lock_guard<std::mutex> lock(modelDataDeleteMutex);
    if (id < modelDatas_.size() && modelDatas_[id]->inUse) {
        modelDataReleaseQueue.emplace_back(id, MAX_FRAME + 1);
    }
}

void vulkanFrameWork::DeleteCompMaterialData(uint32_t id) {
    std::lock_guard<std::mutex> lock(compMaterialDeleteMutex);
    if (id < compMaterialDatas_.size() && compMaterialDatas_[id]->inUse) {
        compMaterialDataReleaseQueue.emplace_back(id, MAX_FRAME + 1);
    }
}

void vulkanFrameWork::CheckDelete() {
    processReleaseQueue<FrameWork::Texture>(textureReleaseQueue);
    processReleaseQueue<FrameWork::Mesh>(meshReleaseQueue);
    processReleaseQueue<FrameWork::VulkanAttachment>(attachmentReleaseQueue);
    processReleaseQueue<FrameWork::VulkanFBO>(fboReleaseQueue);
    processReleaseQueue<FrameWork::VulkanPipeline>(pipelineReleaseQueue);
    processReleaseQueue<FrameWork::MaterialData>(materialDataReleaseQueue);
    processReleaseQueue<FrameWork::VulkanModelData>(modelDataReleaseQueue);
    processReleaseQueue<FrameWork::CompMaterialData>(compMaterialDataReleaseQueue);
}

void vulkanFrameWork::SetWindowResizedCallBack(const WindowResizedCallback &callback) {
    windowResizedCallbacks.push_back(callback);
}



void vulkanFrameWork::setupRenderPass() {

    VkRenderPass renderPass;
    {
        std::array<VkAttachmentDescription, 2> attachments = {};
        // Color attachment
        attachments[0].format = swapChain.colorFormat;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        // Depth attachment
        attachments[1].format = depthFormat;
        attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


        VkAttachmentReference colorReference = {};
        colorReference.attachment = 0;
        colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthReference = {};
        depthReference.attachment = 1;
        depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpassDescription = {};
        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.colorAttachmentCount = 1;
        subpassDescription.pColorAttachments = &colorReference;
        subpassDescription.pDepthStencilAttachment = &depthReference;
        subpassDescription.inputAttachmentCount = 0;
        subpassDescription.pInputAttachments = nullptr;
        subpassDescription.preserveAttachmentCount = 0;
        subpassDescription.pPreserveAttachments = nullptr;
        subpassDescription.pResolveAttachments = nullptr;

        // Subpass dependencies for layout transitions
        std::array<VkSubpassDependency, 2> dependencies{};
        //Access Mask 会指向附件类型

        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        dependencies[0].dependencyFlags = 0;

        dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].dstSubpass = 0;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].srcAccessMask = 0;
        dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        dependencies[1].dependencyFlags = 0;



        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpassDescription;
        renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
        renderPassInfo.pDependencies = dependencies.data();

        VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));

        renderPasses["forward"] = renderPass;
        renderPassTable[RenderPassType::Forward] = renderPass;
    }

    //MSAA
    {
        std::array<VkAttachmentDescription, 3> attachments = {};
        // Color attachment
        attachments[0].format = swapChain.colorFormat;
        attachments[0].samples = msaaSamples;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        // Depth attachment
        attachments[1].format = depthFormat;
        attachments[1].samples = msaaSamples;
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        attachments[2].format = swapChain.colorFormat;
        attachments[2].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[2].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkAttachmentReference colorReference = {};
        colorReference.attachment = 0;
        colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthReference = {};
        depthReference.attachment = 1;
        depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference resolveReference = {};
        resolveReference.attachment = 2;
        resolveReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;



        VkSubpassDescription subpassDescription = {};
        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.colorAttachmentCount = 1;
        subpassDescription.pColorAttachments = &colorReference;
        subpassDescription.pDepthStencilAttachment = &depthReference;
        subpassDescription.inputAttachmentCount = 0;
        subpassDescription.pInputAttachments = nullptr;
        subpassDescription.preserveAttachmentCount = 0;
        subpassDescription.pPreserveAttachments = nullptr;
        subpassDescription.pResolveAttachments = &resolveReference;

        // Subpass dependencies for layout transitions
        std::array<VkSubpassDependency, 2> dependencies{};
        //Access Mask 会指向附件类型

        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        dependencies[0].dependencyFlags = 0;

        dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].dstSubpass = 0;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].srcAccessMask = 0;
        dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        dependencies[1].dependencyFlags = 0;



        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpassDescription;
        renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
        renderPassInfo.pDependencies = dependencies.data();

        VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
        renderPasses["forwardMSAA"] = renderPass;
        renderPassTable[RenderPassType::MsaaForward] = renderPass;
    }
    //Present
    {
        VkAttachmentDescription colorAttachmentDescription = {
            .flags = 0,
            .format = swapChain.colorFormat,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        };

        VkAttachmentReference colorAttachmentRef = {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        };
        std::vector colorAttachments = {
            colorAttachmentRef,
        };

        VkSubpassDescription subpassDescription = {
            .flags = 0,
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .inputAttachmentCount = 0,
            .pInputAttachments = nullptr,
            .colorAttachmentCount = 1,
            .pColorAttachments = colorAttachments.data(),
            .pResolveAttachments = nullptr,
            .pDepthStencilAttachment = nullptr,
            .preserveAttachmentCount = 0,
            .pPreserveAttachments = nullptr
        };

        VkSubpassDependency dependency = {
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .dependencyFlags = 0
        };

        VkRenderPassCreateInfo renderPassInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .attachmentCount = 1,
            .pAttachments = &colorAttachmentDescription,
            .subpassCount = 1,
            .pSubpasses = &subpassDescription,
            .dependencyCount = 1,
            .pDependencies = &dependency
        };

        VkRenderPass presentRenderPass = VK_NULL_HANDLE;
        VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &presentRenderPass));
        renderPassTable[RenderPassType::Present] = presentRenderPass;
    }

}
//虚函数
void vulkanFrameWork::getEnabledFeatures() {
}

void vulkanFrameWork::getEnabledExtensions() {
}

void vulkanFrameWork::prepare() {
    createSurface();
    createSwapChain();
    CreateSwapChainTex();
    createCommandBuffers();
    createSynchronizationPrimitives();
    createPipelineCache();
}


void vulkanFrameWork::windowResize() {
    int _width = 0, _height = 0;
    glfwGetFramebufferSize(window, &_width, &_height);
    windowWidth = _width;
    windowHeight = _height;
    while (windowWidth == 0 || windowHeight == 0) {
        glfwGetFramebufferSize(window, &_width, &_height);
        windowWidth = _width;
        windowHeight = _height;
        glfwWaitEvents();
    }
    //传递变换后的窗口尺寸

    LOG_DEBUG(" windowResize : width {}, height {}", windowWidth, windowHeight);

    vkDeviceWaitIdle(device);
    //重建交换链
    createSwapChain();

    // RecreateAllWindowFrameBuffers();
    // SwitchPresentColorAttachment(presentColorAttachmentID);

    //RecreateSwapchianTexture
    for (int i = 0; i < swapChain.images.size(); i++) {
        auto texture = getByIndex<FrameWork::Texture>(swapChainTextures[i]);
        texture->imageView = swapChain.imageViews[i];
        texture->image.image = swapChain.images[i];
        texture->image.layout = VK_IMAGE_LAYOUT_UNDEFINED;
        texture->image.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        texture->image.format = swapChain.colorFormat;
        texture->isSwapChainRef = true;
        texture->inUse = true;
    }



    for (auto& windowResizedCallback : windowResizedCallbacks) {
        if (windowResizedCallback != nullptr) {
            windowResizedCallback();
        }
    }

    vkDeviceWaitIdle(device);

}


void vulkanFrameWork::prepareFrame(double deltaMilliTime) {
    vkWaitForFences(device, 1, &waitFences[currentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &waitFences[currentFrame]);
    frameCounter++;
    frameTimer = deltaMilliTime /1000.0f;

    //     // 计时，一秒钟，更新一次
    frameCountTimeStamp += deltaMilliTime;
    if (frameCountTimeStamp > 1000.0f) {
        lastFPS = static_cast<uint32_t>(static_cast<float>(frameCounter) * (1000.0f / frameCountTimeStamp));

        glfwSetWindowTitle(window, getWindowTitle().c_str());

        frameCounter = 0;
        frameCountTimeStamp = 0;
    }
    VkResult result = swapChain.acquireNextImage(semaphores.presentComplete[currentFrame], imageIndex);
    // 如果交换链与表面不再兼容（OUT_OF_DATE），则重新创建交换链
    // SRS - 如果不再是最优状态（VK_SUBOPTIMAL_KHR），等到 submitFrame() 中再处理，
    // 以防调整大小时交换链图像数量发生变化
    if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            windowResize();
        }
        return;
    }
    else {
        VK_CHECK_RESULT(result);
    }
    vkResetCommandBuffer(drawCmdBuffers[currentFrame], 0);
}

void vulkanFrameWork::submitFrame() {
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &semaphores.presentComplete[currentFrame];
    //等待上一个渲染的哪个阶段完成
    submitInfo.pWaitDstStageMask = &submitPipelineStages;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &semaphores.renderComplete[currentFrame];
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &drawCmdBuffers[currentFrame];
    VK_CHECK_RESULT(vkQueueSubmit(graphicsQueue, 1, &submitInfo, waitFences[currentFrame]));
    auto result = swapChain.queuePresent(graphicsQueue, imageIndex, semaphores.renderComplete[currentFrame]);//渲染完成在呈现
    // Recreate the swapchain if it's no longer compatible with the surface (OUT_OF_DATE) or \
    // no longer optimal for presentation (SUBOPTIMAL)
    if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {
        windowResize();
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            return;
        }
    }
    else {
        VK_CHECK_RESULT(result);
    }
    vulkanDescriptorPool.ClearPendingQueue();
    CheckDelete(); //帧尾释放一定资源

    currentFrame = (currentFrame + 1) % MAX_FRAME;
}