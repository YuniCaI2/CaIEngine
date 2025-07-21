//
// Created by 51092 on 25-5-6.
//

#include "vulkanFrameWork.h"

#include <iostream>
#include <ostream>
#include<algorithm>
#include <vcruntime_startup.h>

#include <array>
#include <queue>
#include <vector>

#include "VulkanDebug.h"
#include "VulkanTool.h"
#include "VulkanWindow.h"
#define VOLK_IMPLEMENTATION



std::string vulkanFrameWork::getWindowTitle() const {
    std::string windowTitle = {title + "-" + deviceProperties.deviceName};
    if (!settings.overlay) {
        windowTitle += "-" + std::to_string(frameCounter) + "fps";
        windowTitle += "-frameTimer:";
        windowTitle += std::to_string(frameTimer);
    }
    return windowTitle;
}


//主要为渲染场景，其中render的内容需要自己写
void vulkanFrameWork::nextFrame() {

    frameCounter++;
    auto tEnd = std::chrono::high_resolution_clock::now();
    auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tPrevEnd).count();
    frameTimer = static_cast<double>(tDiff) / 1000.0f;
    //测试

    inputManager->update();
    camera.update(frameTimer);

    render(); //渲染
    // Convert to clamped timer value
    if (!paused)
    {
        timer += timerSpeed * frameTimer;
        if (timer > 1.0)
        {
            timer -= 1.0f;
        }
    }
    // 计时，一秒钟，更新一次
    float fpsTimer = (float)(std::chrono::duration<double, std::milli>(tEnd - lastTimestamp).count());
    if (fpsTimer > 1000.0f) {
        lastFPS = static_cast<uint32_t>(static_cast<float>(frameCounter) * (1000.0f / fpsTimer));

        if (!settings.overlay) {
            glfwSetWindowTitle(window, getWindowTitle().c_str());
        }
        frameCounter = 0;
        lastTimestamp = tEnd;
    }
    tPrevEnd = tEnd;
}


void vulkanFrameWork::createPipelineCache() {
    VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
    pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    VK_CHECK_RESULT(vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &pipelineCache));
}

void vulkanFrameWork::createCommandPool() {
    VkCommandPoolCreateInfo cmdPoolInfo = {};
    cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.queueFamilyIndex = swapChain.queueNodeIndex;
    cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK_RESULT(vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &commandPool));
}

//信号量在initVulkan中已经进行了设置
void vulkanFrameWork::createSynchronizationPrimitives() {
    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    //Fence
    waitFences.resize(MAX_FRAME);
    for (size_t i = 0; i < waitFences.size(); i++) {
        VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &waitFences[i]));
    }
    waitFences.resize(MAX_FRAME);
}

void vulkanFrameWork::createSurface() {
    swapChain.initSurface(window);
}

void vulkanFrameWork::createSwapChain() {
    swapChain.create(windowWidth, windowHeight, settings.vsync, settings.fullscreen);
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

vulkanFrameWork::~vulkanFrameWork() {
    //删除池中的内容,完全释放
    //高层向低层释放
    for (int i = 0; i < vulkanPipelines.size(); i++) {
        if (vulkanPipelines[i] != nullptr) {
            destroyByIndex<FrameWork::VulkanPipeline>(i);
            delete vulkanPipelines[i];
            vulkanPipelines[i] = nullptr;
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
    for (auto& [_, r] : renderPasses) {
       if (r != VK_NULL_HANDLE) {
           vkDestroyRenderPass(device, r, nullptr);
       }
    }
    // Clean up Vulkan resources
    swapChain.cleanup();
    vulkanDescriptorPool.DestroyDescriptorPool();
    if (descriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    }
    destroyCommandBuffers();

    // for (auto& frameBuffer : frameBuffers)
    // {
    //     vkDestroyFramebuffer(device, frameBuffer, nullptr);
    // }
    //
    for (auto& shaderModule : shaderModules)
    {
        vkDestroyShaderModule(device, shaderModule, nullptr);
    }
    if (depthStencil.view != VK_NULL_HANDLE)
        vkDestroyImageView(device, depthStencil.view, nullptr);
    vkDestroyImage(device, depthStencil.image, nullptr);
    vkFreeMemory(device, depthStencil.memory, nullptr);

    vkDestroyPipelineCache(device, pipelineCache, nullptr);

    vkDestroyCommandPool(device, commandPool, nullptr);
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

bool vulkanFrameWork::initVulkan() {
    // Instead of checking for the command line switch, validation can be forced via a define
    //设置验证层
    this->settings.validation = true;

    //获得外部服务
    inputManager = FrameWork::Locator::GetService<FrameWork::InputManager>();
    resourceManager = FrameWork::Locator::GetService<FrameWork::Resource>();

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



    //给派生类接口可以添加特性
    getEnabledFeatures();
    //设置各项异性
    if (deviceFeatures.samplerAnisotropy) {
        enabledFeatures.samplerAnisotropy = deviceFeatures.samplerAnisotropy;
    }

    vulkanDevice = new FrameWork::VulkanDevice(physicalDevice, instance);

    //留给派生类的接口可以添加扩展
    getEnabledExtensions();

    result = vulkanDevice->createLogicalDevice(enabledFeatures, enabledDeviceExtensions, deviceCreatepNextChain);
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
    return true;
}

bool vulkanFrameWork::setWindow() {

    window = FrameWork::VulkanWindow::GetInstance().GetWindow();
    return true;
}

VkResult vulkanFrameWork::createInstance() {

    volkInitialize();

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
    if (result == VK_SUCCESS) {
        volkLoadInstance(instance);
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

void vulkanFrameWork::finishRender() {
    //保证渲染资源退出后同步，可以一起删除
    if (device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device);
    }
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


// 这里之创建常规的纹理，不创建天空盒类型的纹理
void vulkanFrameWork::CreateTexture(uint32_t &textureId, FrameWork::TextureFullData& data) {
    FrameWork::Buffer stagingBuffer;
    vulkanDevice->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &stagingBuffer, data.width *data.height * data.numChannels , data.data);
    textureId = getNextIndex<FrameWork::Texture>();
    auto texture = getByIndex<FrameWork::Texture>(textureId);
    auto mipmapLevels = VulkanTool::GetMipMapLevels(data.width, data.height);
    VkFormat  format = VK_FORMAT_R8G8B8A8_SRGB;
    if (data.numChannels == 3) {
        if (data.isRGB) {
            format = VK_FORMAT_R8G8B8_SRGB;
        }else {
            format = VK_FORMAT_B8G8R8_UNORM;
        }
    }
    if (data.numChannels == 4) {
        if (data.isRGB) {
            format = VK_FORMAT_R8G8B8A8_SRGB;
        }else {
            format = VK_FORMAT_B8G8R8A8_UNORM;
        }
    }
    if (data.numChannels == 1) {
        format = VK_FORMAT_R8_UNORM;
    }
    //这里为了统一性array的加载只支持单层的，如果多层可以使用copy的方法叠起来
    vulkanDevice->createImage(&texture->image, VkExtent2D(data.width, data.height), mipmapLevels, 1,VK_SAMPLE_COUNT_1_BIT, format, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    auto cmd = VulkanTool::beginSingleTimeCommands(vulkanDevice->logicalDevice, vulkanDevice->commandPool);
    VulkanTool::transitionImageLayout(cmd,texture->image.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    VulkanTool::endSingleTimeCommands(device, graphicsQueue, commandPool, cmd);

    vulkanDevice->copyBufferToImage(&stagingBuffer, &texture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, graphicsQueue);

    VulkanTool::GenerateMipMaps(*vulkanDevice, texture->image);

    //在生成mipmap的时候会将布局转换成着色器可读
    texture->image.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    stagingBuffer.destroy();

    CreateImageView(texture->image, texture->imageView, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D);
    texture->sampler = CreateSampler(mipmapLevels);

    texture->inUse = true;
    //使用
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

    viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
    viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
    viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
    viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;

    VK_CHECK_RESULT(vkCreateImageView(device, &viewCreateInfo, nullptr, &imageView));
}

void vulkanFrameWork::CreateAttachment(uint32_t &attachmentId, uint32_t width, uint32_t height, AttachmentType attachmentType, VkSampleCountFlagBits numSample, bool isSampled) {
    attachmentId = getNextIndex<FrameWork::VulkanAttachment>();
    auto attachment = getByIndex<FrameWork::VulkanAttachment>(attachmentId);
    attachment->type = attachmentType;
    attachment->width = width;
    attachment->height = height;
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
    else if (attachmentType == AttachmentType::Present) {
        attachment->attachmentsArray.resize(swapChain.imageViews.size());
        attachment->inUse = true;
        for (int i = 0; i < swapChain.imageViews.size(); i++) {
            attachment->attachmentsArray[i] = getNextIndex<FrameWork::Texture>();
            auto tex = getByIndex<FrameWork::Texture>(attachment->attachmentsArray[i]);
            tex->inUse = true;
            tex->sampler = VK_NULL_HANDLE;//不需要
            tex->imageView = swapChain.imageViews[i];
            //这里只要获得引用则可，用于构建帧缓冲，生命周期由交换链管理
        }
    }
}

void vulkanFrameWork::CreateFrameBuffer(uint32_t &frameBufferId, std::vector<uint32_t> &attachments, VkRenderPass renderPass) {
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
    fbo->inUse = true;
    fbo->framebuffers.resize(MAX_FRAME);
    fbo->AttachmentsIdx = attachments;
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

void vulkanFrameWork::CreatePresentFrameBuffer(uint32_t &frameBufferId, uint32_t attachment,
    VkRenderPass renderPass) {
    auto attachmentObj = getByIndex<FrameWork::VulkanAttachment>(attachment);
    if (attachmentObj->type != AttachmentType::Present) {
        std::cerr << "When you use CreatePresentFrameBuffer, you should input atttachment that' s type is Present !" << std::endl;
        exit(-1);
    }
    frameBufferId = getNextIndex<FrameWork::VulkanFBO>();
    auto fbo = getByIndex<FrameWork::VulkanFBO>(frameBufferId);
    fbo->inUse = true;
    fbo->framebuffers.resize(swapChain.imageViews.size());
    fbo->AttachmentsIdx = std::vector {
        attachment,
    };
    for (uint32_t i = 0; i < swapChain.images.size(); i++) {
        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.width = getByIndex<FrameWork::VulkanAttachment>(attachment)->width;
        framebufferInfo.height = getByIndex<FrameWork::VulkanAttachment>(attachment)->height;
        framebufferInfo.layers = 1;
        framebufferInfo.pAttachments = &getByIndex<FrameWork::Texture>(attachmentObj->attachmentsArray[i])->imageView;
        VK_CHECK_RESULT(vkCreateFramebuffer(device, &framebufferInfo, nullptr,&fbo->framebuffers[i]));
    }
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

void vulkanFrameWork::
LoadPipelineShader(uint32_t &pipelineInfoIdx, const std::string &fileName, VkShaderStageFlagBits stage) {
    auto pipelineInfo = getByIndex<FrameWork::VulkanPipelineInfo>(pipelineInfoIdx);
    pipelineInfo->shaderModules[stage] = loadShader(fileName, stage);
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

void vulkanFrameWork::CreateVulkanPipeline(uint32_t& pipelineIdx, const std::string &name, uint32_t& pipelineInfoIdx,
    const std::string &renderPassName, uint32_t subpassIndex, const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts) {
    pipelineIdx = getNextIndex<FrameWork::VulkanPipeline>();
    auto vulkanPipeline = getByIndex<FrameWork::VulkanPipeline>(pipelineIdx);
    vulkanPipeline->inUse = true;
    auto vulkanPipelineInfo = getByIndex<FrameWork::VulkanPipelineInfo>(pipelineInfoIdx);
    vulkanPipelineInfo->inUse = true;
    vulkanPipeline->descriptorSetLayouts = descriptorSetLayouts;
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .pNext = nullptr,
    .setLayoutCount = (uint32_t)descriptorSetLayouts.size(),
    .pSetLayouts = descriptorSetLayouts.data(),
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
            .pVertexBindingDescriptions = vulkanPipelineInfo->vertexBindingDescriptions.size() == 0 ? nullptr : vulkanPipelineInfo->vertexBindingDescriptions.data(),
            .vertexAttributeDescriptionCount = static_cast<uint32_t>(vulkanPipelineInfo->vertexAttributeDescriptions.size()),
            .pVertexAttributeDescriptions = vulkanPipelineInfo->vertexAttributeDescriptions.size() == 0 ? nullptr : vulkanPipelineInfo->vertexAttributeDescriptions.data(),
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

    vulkanPipeline->pipelineInfoIdx = pipelineInfoIdx;
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &vulkanPipeline->pipeline));
}


void vulkanFrameWork::InitPresent(const std::string &presentShaderName, uint32_t colorAttachmentID) {
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
    RegisterRenderPass(presentRenderPass, "presentRenderPass");

    auto presentDescriptorLayout = CreateDescriptorSetLayout(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);

    uint32_t attachment = -1;
    CreateAttachment(attachment, windowWidth, windowHeight, AttachmentType::Present, VK_SAMPLE_COUNT_1_BIT, false);
    CreatePresentFrameBuffer(presentFrameBufferIndex, attachment, presentRenderPass);
    //管线的创建 ---------------------------------------------------------------------------------------------------------

    uint32_t presentPipelineInfoIdx = 0;
    InitPipelineInfo(presentPipelineInfoIdx);
    LoadPipelineShader(presentPipelineInfoIdx, presentShaderName, VK_SHADER_STAGE_VERTEX_BIT);
    LoadPipelineShader(presentPipelineInfoIdx, presentShaderName, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkViewport viewport = {
        .x = 0,
        .y = 0,
        .width = (float)windowWidth,
        .height = (float)windowHeight,
        .minDepth = 0,
        .maxDepth = 1
    };
    SetPipelineViewPort(presentPipelineInfoIdx, viewport);
    VkRect2D scissor = {
        .offset = {0,0},
        .extent = {windowWidth,windowHeight}
    };
    SetPipelineScissor(presentPipelineInfoIdx, scissor);
    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0,
        .depthBiasClamp = 0,
        .depthBiasSlopeFactor = 0,
        .lineWidth = 1.0f,
    };
    SetPipelineRasterizationState(presentPipelineInfoIdx, rasterizer);
    VkPipelineMultisampleStateCreateInfo multisampleState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE, //解决了纹理高频变化
        .minSampleShading = 0.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE
    };
    SetPipelineMultiSampleState(presentPipelineInfoIdx, multisampleState);
    VkPipelineDepthStencilStateCreateInfo depthStencilState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthTestEnable = VK_FALSE, //呈现纹理不需要深度测试
        .depthWriteEnable = VK_FALSE,
        .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
    };
    SetPipelineDepthStencilState(presentPipelineInfoIdx, depthStencilState);
    AddPipelineColorBlendState(presentPipelineInfoIdx, true, BlendOp::Opaque);
    CreateVulkanPipeline(presentPipelineIndex, "presentPipeline", presentPipelineInfoIdx,
        "presentRenderPass", 0, {presentDescriptorLayout});

    //管线创建------------------------------------------------------------------------------------------------------------

    auto colorAttachment = getByIndex<FrameWork::VulkanAttachment>(colorAttachmentID);
    std::vector<FrameWork::TextureFullData> textureDatas(colorAttachment->attachmentsArray.size());
    for (int i = 0; i < colorAttachment->attachmentsArray.size(); i++) {
        textureDatas[i].textureID = colorAttachment->attachmentsArray[i];
        //将纹理信息进行传递
    }
    std::vector descriptorSetLayouts(textureDatas.size(), presentDescriptorLayout);
    FrameWork::MaterialCreateInfo materialInfo = {
        .UniformDescriptorLayouts = {},
        .TexturesDescriptorLayouts = descriptorSetLayouts,
        .UniformData = {},
        .TexturesDatas = textureDatas,
    };
    CreateMaterial(presentMaterialIndex, materialInfo);
    textureDatas.clear();
}

void vulkanFrameWork::PresentFrame(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkClearValue clearValue[2];
    clearValue[0].color = defaultClearColor;
    clearValue[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,
        .renderPass = GetRenderPass("presentRenderPass"),
        .framebuffer = getByIndex<FrameWork::VulkanFBO>(presentFrameBufferIndex)->framebuffers[imageIndex],
        .renderArea = {
            .offset = {0, 0},
            .extent = {
                .width = windowWidth,
                .height = windowHeight,
            }
        },
        .clearValueCount = 2,
        .pClearValues = clearValue
    };
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {};
    viewport.width = (float) windowWidth;
    viewport.height = (float) windowHeight;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.extent.width = windowWidth;
    scissor.extent.height = windowHeight;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, getByIndex<FrameWork::VulkanPipeline>(presentPipelineIndex)->pipeline);
    auto material = getByIndex<FrameWork::Material>(presentMaterialIndex);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        getByIndex<FrameWork::VulkanPipeline>(presentPipelineIndex)->pipelineLayout, 0,
        1, &material->descriptorSets[GetCurrentFrame()]
        , 0, nullptr
        );

    //呈现的顶点硬编码在着色器中
    vkCmdDraw(commandBuffer, 6, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);
}

VkDescriptorSetLayout vulkanFrameWork::CreateDescriptorSetLayout(
        VkDescriptorType descriptorType, uint32_t descriptorCount, VkShaderStageFlags stageFlags
    ) {
    VkDescriptorSetLayoutBinding binding = {
        .binding = 0,
        .descriptorType = descriptorType,
        .descriptorCount = descriptorCount,
        .stageFlags = stageFlags,
        .pImmutableSamplers = nullptr,
    };
    std::vector layoutBindings= {
        binding,
    };
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = (uint32_t)layoutBindings.size(),
        .pBindings = layoutBindings.data(),
    };
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutInfo, nullptr, &descriptorSetLayout));
    return descriptorSetLayout;
}

void vulkanFrameWork::CreateMaterial(uint32_t &materialIdx, FrameWork::MaterialCreateInfo &materialInfo) {
    materialIdx = getNextIndex<FrameWork::Material>();
    auto material = getByIndex<FrameWork::Material>(materialIdx);
    material->inUse = true;
    material->uniformBuffer.resize(materialInfo.UniformData.size());
    material->uniformBufferSizes.resize(materialInfo.UniformData.size());
    material->textures.resize(materialInfo.TexturesDatas.size());
    std::vector<std::pair<VkDescriptorSet, VkDescriptorSetLayout>> uniformDescriptorSets(materialInfo.UniformData.size());
    std::vector<std::pair<VkDescriptorSet, VkDescriptorSetLayout>> texturesDescriptorSets(materialInfo.TexturesDatas.size());
    auto alignment = vulkanDevice->properties.limits.minUniformBufferOffsetAlignment;
    for (int i = 0 ; i < material->uniformBuffer.size(); i++) {
        uint32_t dynamicSize = materialInfo.UniformData[i].second & ~ (alignment - 1);
        material->uniformBufferSizes[i] = dynamicSize * MAX_FRAME;
        vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            ,&material->uniformBuffer[i], dynamicSize * MAX_FRAME
            );
        material->uniformBuffer[i].map();
        for (int k = 0; k < MAX_FRAME; k++) {
            memcpy(static_cast<char*>(material->uniformBuffer[i].mapped) + dynamicSize * k, materialInfo.UniformData[i].first, dynamicSize);
        }
        vulkanDescriptorPool.AllocateDescriptorSet(materialInfo.UniformDescriptorLayouts[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, uniformDescriptorSets[i].first);
        uniformDescriptorSets[i].second = materialInfo.UniformDescriptorLayouts[i];
        VkDescriptorBufferInfo uniformBufferInfo = {
            .buffer = material->uniformBuffer[i].buffer,
            .offset = 0,
            .range = materialInfo.UniformData[i].second
        };

        VkWriteDescriptorSet descriptorWrite = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = uniformDescriptorSets[i].first,
            .dstBinding = 0,
            .dstArrayElement = 0,//描述符元素数组开始的下标
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            .pImageInfo = nullptr,
            .pBufferInfo = &uniformBufferInfo,
            .pTexelBufferView = nullptr,
        };
        vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
    }

    for (int i = 0; i < material->textures.size(); i++) {
        VkDescriptorImageInfo textureImageInfo;
        if (materialInfo.TexturesDatas[i].textureID.has_value()) {
            material->textures[i] = materialInfo.TexturesDatas[i].textureID.value();
            auto texture = getByIndex<FrameWork::Texture>(material->textures[i]);
            vulkanDescriptorPool.AllocateDescriptorSet(materialInfo.TexturesDescriptorLayouts[i],
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, texturesDescriptorSets[i].first);
            textureImageInfo = {
                .sampler = texture->sampler,
                .imageView = texture->imageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,//保证在使用的时候是这个布局
            };
        }else {
            material->textures[i] = getNextIndex<FrameWork::Texture>();
            auto texture = getByIndex<FrameWork::Texture>(material->textures[i]);
            CreateTexture(material->textures[i], materialInfo.TexturesDatas[i]);
            texturesDescriptorSets[i].second = materialInfo.TexturesDescriptorLayouts[i];
            textureImageInfo = {
                .sampler = texture->sampler,
                .imageView = texture->imageView,
                .imageLayout =VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };
        }

        VkWriteDescriptorSet descriptorWrite = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = texturesDescriptorSets[i].first,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType =  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &textureImageInfo,
            .pBufferInfo = nullptr,
            .pTexelBufferView = nullptr,
        };
        vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
    }
    material->descriptorPairs = std::move(uniformDescriptorSets);
    material->descriptorPairs.insert(material->descriptorPairs.end(), texturesDescriptorSets.begin(), texturesDescriptorSets.end());
    material->descriptorSets.resize(material->descriptorPairs.size());
    for (int i = 0; i < material->descriptorPairs.size(); i++) {
        material->descriptorSets[i] = material->descriptorPairs[i].first;
    }
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
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
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
        throw std::runtime_error(name + "render pass not found!");
    }else {
        return renderPasses.at(name);
    }
}

double vulkanFrameWork::GetFrameTime() const {
    return frameTimer;
}


void vulkanFrameWork::windowResized() {
}//虚函数

void vulkanFrameWork::buildCommandBuffers() {
}//虚函数

void vulkanFrameWork::setupDepthStencil() {
    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.extent.width = windowWidth;
    imageCreateInfo.extent.height = windowHeight;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.format = depthFormat;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    //设置采样数

    VK_CHECK_RESULT(vkCreateImage(device, &imageCreateInfo, nullptr, &depthStencil.image));
    VkMemoryRequirements memRequirements{};
    vkGetImageMemoryRequirements(device, depthStencil.image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &depthStencil.memory));
    VK_CHECK_RESULT(vkBindImageMemory(device, depthStencil.image, depthStencil.memory, 0));

    VkImageViewCreateInfo viewCreateInfo{};
    viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCreateInfo.image = depthStencil.image;
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCreateInfo.format = depthFormat;
    viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewCreateInfo.subresourceRange.baseMipLevel = 0;
    viewCreateInfo.subresourceRange.levelCount = 1;
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;
    viewCreateInfo.subresourceRange.layerCount = 1;
    if (depthFormat >= VK_FORMAT_D16_UNORM_S8_UINT) {
        //将模板的aspectMask也加入
        viewCreateInfo.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    VK_CHECK_RESULT(vkCreateImageView(device, &viewCreateInfo, nullptr, &depthStencil.view));
}


void vulkanFrameWork::setupFrameBuffer() {
    // Create frame buffers for every swap chain image
    // frameBuffers.resize(swapChain.images.size());
    // for (uint32_t i = 0; i < frameBuffers.size(); i++)
    // {
    //     //两个附件一个是交换链一个是设置深度
    //     const VkImageView attachments[2] = {
    //         swapChain.imageViews[i],
    //         // Depth/Stencil attachment is the same for all frame buffers
    //         depthStencil.view
    //     };
    //     VkFramebufferCreateInfo frameBufferCreateInfo{};
    //     frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    //     frameBufferCreateInfo.renderPass = renderPass;
    //     frameBufferCreateInfo.attachmentCount = 2;
    //     frameBufferCreateInfo.pAttachments = attachments;
    //     frameBufferCreateInfo.width = windowWidth;
    //     frameBufferCreateInfo.height = windowHeight;
    //     frameBufferCreateInfo.layers = 1;
    //     VK_CHECK_RESULT(vkCreateFramebuffer(device, &frameBufferCreateInfo, nullptr, &frameBuffers[i]));
    // }
}

//虚函数
void vulkanFrameWork::setupRenderPass() {

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
	std::array<VkSubpassDependency, 3> dependencies{};
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

    dependencies[2].srcSubpass = 0;
    dependencies[2].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[2].srcAccessMask =  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
    dependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[2].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[2].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[2].dependencyFlags = 0;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpassDescription;
	renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
	renderPassInfo.pDependencies = dependencies.data();

	VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
}
//虚函数
void vulkanFrameWork::getEnabledFeatures() {
}

void vulkanFrameWork::getEnabledExtensions() {
}

void vulkanFrameWork::prepare() {
    createSurface();
    createCommandPool();
    createSwapChain();
    createCommandBuffers();
    createSynchronizationPrimitives();
    setupDepthStencil();
    setupRenderPass();
    createPipelineCache();
    setupFrameBuffer();
}

VkPipelineShaderStageCreateInfo vulkanFrameWork::loadShader(const std::string &fileName, VkShaderStageFlagBits stage) {
    VkPipelineShaderStageCreateInfo shaderStageInfo{};
    shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.stage = stage;
    shaderStageInfo.pName = "main";
    shaderStageInfo.module = resourceManager->getShaderModulFromFile(device, fileName, stage);
    assert(shaderStageInfo.module != VK_NULL_HANDLE);
    shaderModules.emplace_back(shaderStageInfo.module);
    return shaderStageInfo;
}

void vulkanFrameWork::windowResize() {
    int _width = 0, _height = 0;
    glfwGetFramebufferSize(window, &_width, &_height);
    while (windowWidth == 0 || windowHeight == 0) {
        glfwGetFramebufferSize(window, &_width, &_height);
        glfwWaitEvents();
    }
    //传递变换后的窗口尺寸
    destWidth = _width;
    destHeight = _height;

    vkDeviceWaitIdle(device);

    windowWidth = destWidth;
    windowHeight = destHeight;
    createSwapChain();

    // Recreate the frame buffers
    vkDestroyImageView(device, depthStencil.view, nullptr);
    vkDestroyImage(device, depthStencil.image, nullptr);
    vkFreeMemory(device, depthStencil.memory, nullptr);
    setupDepthStencil();

    for (auto& frameBuffer : frameBuffers) {
        vkDestroyFramebuffer(device, frameBuffer, nullptr);
    }
    setupFrameBuffer();

    // Command buffers need to be recreated as they may store
    // references to the recreated frame buffer
    destroyCommandBuffers();
    createCommandBuffers();
    buildCommandBuffers();

    // SRS - Recreate fences in case number of swapchain images has changed on resize
    for (auto& fence : waitFences) {
        vkDestroyFence(device, fence, nullptr);
    }
    createSynchronizationPrimitives();

    vkDeviceWaitIdle(device);



    // Notify derived class
    windowResized();
}

void vulkanFrameWork::renderLoop() {
    lastTimestamp = std::chrono::high_resolution_clock::now();
    tPrevEnd = lastTimestamp;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            //因为鼠标在第一人称的时候会被隐藏，
            //后续实现Input模块和实现鼠标的显示和切换的时候则处理
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        nextFrame();
    }
    if (device != VK_NULL_HANDLE) {
        //保证资源同步退出循环可以被删除
        vkDeviceWaitIdle(device);
    }
}



void vulkanFrameWork::prepareFrame() {
    vkWaitForFences(device, 1, &waitFences[currentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &waitFences[currentFrame]);
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
}

void vulkanFrameWork::submitFrame() {
    vkResetCommandBuffer(drawCmdBuffers[currentFrame], 0);
    buildCommandBuffers();
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
    auto result = swapChain.queuePresent(graphicsQueue, imageIndex, semaphores.renderComplete[currentFrame]);
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
    currentFrame = (currentFrame + 1) % MAX_FRAME;
}