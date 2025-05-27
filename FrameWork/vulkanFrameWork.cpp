//
// Created by 51092 on 25-5-6.
//

#include "vulkanFrameWork.h"

#include <iostream>
#include <ostream>
#include<algorithm>
#include <vcruntime_startup.h>

#include <array>
#include "VulkanDebug.h"
#include "VulkanTool.h"
#define VOLK_IMPLEMENTATION



std::string vulkanFrameWork::getWindowTitle() const {
    std::string windowTitle = {title + "-" + deviceProperties.deviceName};
    if (!settings.overlay) {
        windowTitle += "-" + std::to_string(frameCounter) + "fps";
    }
    return windowTitle;
}

void vulkanFrameWork::handleMouseMove(int32_t x, int32_t y) {
}

void vulkanFrameWork::nextFrame() {
    auto tStart = std::chrono::high_resolution_clock::now();
    if (viewUpdated) {
        viewUpdated = false;
    }
    render(); //渲染
    frameCounter++;
    auto tEnd = std::chrono::high_resolution_clock::now();
    auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
    frameTimer = static_cast<float>(tDiff) / 1000.0f;
    camera.update(frameTimer);
    if (camera.moving())
    {
        viewUpdated = true; //视图更新为真
    }
    // Convert to clamped timer value
    if (!paused)
    {
        timer += timerSpeed * frameTimer;
        if (timer > 1.0)
        {
            timer -= 1.0f;
        }
    }
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
    //更新UI
    updateOverlay();
}

void vulkanFrameWork::updateOverlay() {
    if (!settings.overlay) {
        return;
    }
    //ui不需要每一帧都更新
    ui.updateTimer -= frameTimer;
    if (ui.updateTimer >= 0.0f) {
        return;
    }
    ui.updateTimer = 1.0f / 30.0f;
    //30帧ui

    ImGuiIO &io = ImGui::GetIO();

    io.DisplaySize = ImVec2((float)width, (float)height);
    io.DeltaTime = frameTimer;

    int leftButton = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    int rightButton = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
    int middleButton = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE);
    float xscale, yscale;
    glfwGetWindowContentScale(window, &xscale, &yscale);
    ui.scale = (xscale + yscale) / 2;

    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);

    io.MousePos = ImVec2(static_cast<float>(mouseX), static_cast<float>(mouseY));
    //可能imgui中会管理按钮是否重复触发
    io.MouseDown[0] = leftButton == GLFW_PRESS;
    io.MouseDown[1] = rightButton == GLFW_PRESS;
    io.MouseDown[2] = middleButton == GLFW_PRESS;

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    //设置窗口不能被调节大小和移动
    ImGui::Begin("CaI Engine FrameWork", nullptr, ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    ImGui::TextUnformatted(title.c_str());
    //显示设备名称
    ImGui::TextUnformatted(deviceProperties.deviceName);
    //显示帧数
    ImGui::Text("%.2f ms/frame (%.1d fps)", (1000.0f / lastFPS), lastFPS);

    ImGui::PushItemWidth(110.0f * ui.scale); //设置ui控件的宽度 ,这是一个参数栈
    OnUpdateUIOverlay(&ui); //自己添加ui , framework 中为空
    ImGui::PopItemWidth(); //弹出刚才的设置

    ImGui::End();
    ImGui::PopStyleVar(); //弹出刚才的样式
    ImGui::Render();

    buildCommandBuffers(); //更新了则重建命令缓冲区
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
    //Fence
    waitFences.resize(drawCmdBuffers.size());
    for (size_t i = 0; i < waitFences.size(); i++) {
        VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &waitFences[i]));
    }
}

void vulkanFrameWork::createSurface() {
    swapChain.initSurface(window);
}

void vulkanFrameWork::createSwapChain() {
    swapChain.create(width, height, settings.vsync, settings.fullscreen);
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
    // Clean up Vulkan resources
    swapChain.cleanup();
    //实际的描述符池应该在派生类中设置
    if (descriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    }
    destroyCommandBuffers();
    if (renderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(device, renderPass, nullptr);
    }
    for (auto& frameBuffer : frameBuffers)
    {
        vkDestroyFramebuffer(device, frameBuffer, nullptr);
    }

    for (auto& shaderModule : shaderModules)
    {
        vkDestroyShaderModule(device, shaderModule, nullptr);
    }
    vkDestroyImageView(device, depthStencil.view, nullptr);
    vkDestroyImage(device, depthStencil.image, nullptr);
    vkFreeMemory(device, depthStencil.memory, nullptr);

    vkDestroyPipelineCache(device, pipelineCache, nullptr);

    vkDestroyCommandPool(device, commandPool, nullptr);

    vkDestroySemaphore(device, semaphores.presentComplete, nullptr);
    vkDestroySemaphore(device, semaphores.renderComplete, nullptr);
    for (auto& fence : waitFences) {
        vkDestroyFence(device, fence, nullptr);
    }

    if (settings.overlay) {
        ui.freeResources();
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

    //初始化,不然后续关于glfw的操作会报错
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

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
    queue = VK_NULL_HANDLE;
    vkGetDeviceQueue(device, vulkanDevice->queueFamilyIndices.graphics, 0, &queue);

    VkBool32 validFormat{false};

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
    VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphores.presentComplete));
    VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphores.renderComplete));

    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &semaphores.presentComplete;
    //等待上一个渲染的哪个阶段完成
    submitInfo.pWaitDstStageMask = &submitPipelineStages;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &semaphores.renderComplete;

    return true;
}

bool vulkanFrameWork::setWindow() {

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    this->window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (window == nullptr) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwSetWindowUserPointer(window, this);
    glfwSetCursorPosCallback(window, vulkanFrameWork::CursorPosCallbackInternal);
    glfwSetMouseButtonCallback(window, vulkanFrameWork::MouseButtonCallbackInternal);
    glfwSetKeyCallback(window, vulkanFrameWork::KeyCallbackInternal);
    glfwSetScrollCallback(window, vulkanFrameWork::ScrollCallbackInternal);
    //还有尺寸调整的回调

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

void vulkanFrameWork::setKeyCallback(KeyCallback callback) {
    this->userKeyCallback = std::move(callback);
}

void vulkanFrameWork::setMouseButtonCallback(MouseButtonCallback callback) {
    this->userMouseButtonCallback = std::move(callback);
}

void vulkanFrameWork::setCursorPosCallback(CursorPosCallback callback) {
    this->userCursorPosCallback = std::move(callback);
}

void vulkanFrameWork::setScrollCallback(ScrollCallback callback) {
    this->userScrollCallback = std::move(callback);
}

//所以要记得先将this指针和window绑定
void vulkanFrameWork::KeyCallbackInternal(GLFWwindow *window, int key, int scancode, int action, int mods) {
    auto* instance = static_cast<vulkanFrameWork*>(glfwGetWindowUserPointer(window));
    if (instance && instance->userKeyCallback) {
        instance->userKeyCallback(window, key, scancode, action, mods);
    }
}

void vulkanFrameWork::MouseButtonCallbackInternal(GLFWwindow *window, int button, int action, int mods) {
    auto* instance = static_cast<vulkanFrameWork*>(glfwGetWindowUserPointer(window));
    if (instance && instance->userMouseButtonCallback) {
        instance->userMouseButtonCallback(window, button, action, mods);
    }
}

void vulkanFrameWork::CursorPosCallbackInternal(GLFWwindow *window, double xpos, double ypos) {
    auto* instance = static_cast<vulkanFrameWork*>(glfwGetWindowUserPointer(window));
    if (instance && instance->userCursorPosCallback) {
        instance->userCursorPosCallback(window, xpos, ypos);
    }
}

void vulkanFrameWork::ScrollCallbackInternal(GLFWwindow *window, double xoffset, double yoffset) {
    auto* instance = static_cast<vulkanFrameWork*>(glfwGetWindowUserPointer(window));
    if (instance && instance->userScrollCallback) {
        instance->userScrollCallback(window, xoffset, yoffset);
    }
}

void vulkanFrameWork::windowResized() {
}//虚函数

void vulkanFrameWork::buildCommandBuffers() {
}//虚函数

void vulkanFrameWork::setupDepthStencil() {
    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.extent.width = width;
    imageCreateInfo.extent.height = height;
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
    frameBuffers.resize(swapChain.images.size());
    for (uint32_t i = 0; i < frameBuffers.size(); i++)
    {
        //两个附件一个是交换链一个是设置深度
        const VkImageView attachments[2] = {
            swapChain.imageViews[i],
            // Depth/Stencil attachment is the same for all frame buffers
            depthStencil.view
        };
        VkFramebufferCreateInfo frameBufferCreateInfo{};
        frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        frameBufferCreateInfo.renderPass = renderPass;
        frameBufferCreateInfo.attachmentCount = 2;
        frameBufferCreateInfo.pAttachments = attachments;
        frameBufferCreateInfo.width = width;
        frameBufferCreateInfo.height = height;
        frameBufferCreateInfo.layers = 1;
        VK_CHECK_RESULT(vkCreateFramebuffer(device, &frameBufferCreateInfo, nullptr, &frameBuffers[i]));
    }
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
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
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

    if (settings.overlay) {
        ui.device = vulkanDevice;
        ui.prepareResources(window, instance, renderPass, swapChain, queue);
    }
}

VkPipelineShaderStageCreateInfo vulkanFrameWork::loadShader(const std::string &fileName, VkShaderStageFlagBits stage) {
    VkPipelineShaderStageCreateInfo shaderStageInfo{};
    shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.stage = stage;
    shaderStageInfo.pName = "main";
    shaderStageInfo.module = VulkanTool::loadShader(fileName, device);
    assert(shaderStageInfo.module != VK_NULL_HANDLE);
    shaderModules.emplace_back(shaderStageInfo.module);
    return shaderStageInfo;
}

void vulkanFrameWork::windowResize() {
    if (!prepared) {
        return;
    }
    prepared = false;
    resized = true;

    int _width = 0, _height = 0;
    glfwGetFramebufferSize(window, &_width, &_height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &_width, &_height);
        glfwWaitEvents();
    }
    //传递变换后的窗口尺寸
    destWidth = _width;
    destHeight = _height;

    vkDeviceWaitIdle(device);

    width = destWidth;
    height = destHeight;
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

    if ((width > 0.0f) && (height > 0.0f)) {
        if (settings.overlay) {
            ui.resize(width, height);
        }
    }

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

    if ((width > 0.0f) && (height > 0.0f)) {
        camera.updateAspectRatio((float)width / (float)height);
    }

    // Notify derived class
    windowResized();

    prepared = true;

}

void vulkanFrameWork::renderLoop() {
    lastTimestamp = std::chrono::high_resolution_clock::now();
    tPrevEnd = lastTimestamp;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        // if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        //     glfwSetWindowShouldClose(window, true);
        // }
        if (prepared)
            nextFrame();
    }
    if (device != VK_NULL_HANDLE) {
        //保证资源同步退出循环可以被删除
        vkDeviceWaitIdle(device);
    }
}

void vulkanFrameWork::drawUI(const VkCommandBuffer &commandBuffer) {
    if (settings.overlay && ui.visible) {
        ui.draw(commandBuffer);
    }
}

void vulkanFrameWork::prepareFrame() {
    VkResult result = swapChain.acquireNextImage(semaphores.presentComplete, currentBuffer);
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
    auto result = swapChain.queuePresent(queue, currentBuffer, semaphores.renderComplete);
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
    VK_CHECK_RESULT(vkQueueWaitIdle(queue));
}

void vulkanFrameWork::renderFrame() {
    prepareFrame();
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
    VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
    submitFrame();
}

void vulkanFrameWork::OnUpdateUIOverlay(FrameWork::VulkanUIOverlay *overlay) {
    //空的
}
