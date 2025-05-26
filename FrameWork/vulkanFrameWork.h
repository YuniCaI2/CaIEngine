//
// Created by 51092 on 25-5-6.
//

#ifndef VULKANFRAMEWORK_H
#define VULKANFRAMEWORK_H

// 标准库包含
#include <string>
#include <vector>
#include <functional>
#include <chrono>

// GLM 定义
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // Vulkan 通常使用这个深度范围
#define GLM_ENABLE_EXPERIMENTAL     // 如果你使用了 GLM 的实验性特性
// GLM 包含
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp> // 如果需要
#include <glm/gtc/type_ptr.hpp>       // 如果需要

// --- Vulkan 配置和头文件 ---
// 1. 定义 VK_NO_PROTOTYPES 来阻止 Vulkan SDK 定义函数符号。
#define VK_NO_PROTOTYPES

// 2. 定义 GLFW_INCLUDE_VULKAN 让 GLFW 为我们包含 <vulkan/vulkan.h>。

#include "pubh.h"

#include "VulkanSwapChain.h"    // 确保这些头文件也不会以冲突的方式包含 vulkan.h。
#include "VulkanUIOverlay.h"    // 它们应该依赖此处的设置或 volk。
#include "Camera.h"




class VulkanSwapChain;
class vulkanFrameWork {
private:
    std::string getWindowTitle() const; //窗口标题
    uint32_t destWidth{};//宽度
    uint32_t destHeight{};//高度
    bool resizing = false;//是否能调整大小
    void handleMouseMove(int32_t x, int32_t y); //处理鼠标移动
    void nextFrame();
    void updateOverlay();//更新屏幕上的覆盖层
    void createPipelineCache();
    void createCommandPool();
    void createSynchronizationPrimitives(); //创建一些同步图元的对象
    void createSurface();
    void createSwapChain();
    void createCommandBuffers();
    void destroyCommandBuffers();
    std::string shaderDir = "glsl";

protected:
    //得到一个绝对路径
    std::string getShaderPath() const;


    //Frame counter to display fps
    uint32_t frameCounter = 0;
    uint32_t lastFPS = 0;
    std::chrono::time_point<std::chrono::high_resolution_clock> lastTimestamp, tPrevEnd;
    // Vulkan instance, store all pre-application states
    VkInstance instance{VK_NULL_HANDLE};
    std::vector<std::string> supportedInstanceExtensions;
    VkPhysicalDevice physicalDevice{VK_NULL_HANDLE};
    VkPhysicalDeviceProperties deviceProperties{};
    VkPhysicalDeviceFeatures deviceFeatures{};
    VkPhysicalDeviceMemoryProperties deviceMemoryProperties{};
    VkPhysicalDeviceFeatures enabledFeatures{};

    std::vector<const char*> enabledDeviceExtensions;
    std::vector<const char*> enabledInstanceExtensions;
    std::vector<VkLayerSettingEXT> enabledLayerSettings;
    //验证层的一些设置信息

    void* deviceCreatepNextChain = nullptr; //扩展结构
    VkDevice device{VK_NULL_HANDLE};
    VkQueue queue{VK_NULL_HANDLE};
    VkFormat depthFormat{VK_FORMAT_UNDEFINED};
    VkCommandPool commandPool{VK_NULL_HANDLE};
    VkPipelineStageFlags submitPipelineStages{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submitInfo{};
    std::vector<VkCommandBuffer> drawCmdBuffers;
    VkRenderPass renderPass{ VK_NULL_HANDLE };
    std::vector<VkFramebuffer>frameBuffers;
    uint32_t currentBuffer{0};
    VkDescriptorPool descriptorPool{ VK_NULL_HANDLE };
    std::vector<VkShaderModule> shaderModules;
    VkPipelineCache pipelineCache{ VK_NULL_HANDLE };
    VulkanSwapChain swapChain;

    //同步信号量
    struct {
        //SwapChain Image Presentation
        VkSemaphore presentComplete{};
        //Command Buffer submission and execution
        VkSemaphore renderComplete{};

    } semaphores{};

    std::vector<VkFence> waitFences;
    bool requiresStencil{false};

public:
    bool prepared{false};
    bool resized{false};
    bool viewUpdated{false}; // 这个是啥

    uint32_t width{1280};
    uint32_t height{720};
    //UI
    FrameWork::VulkanUIOverlay ui;

    float frameTimer = 1.0f;

    FrameWork::VulkanDevice* vulkanDevice{};

    struct Settings {
        //验证层
        bool validation = false;

        bool fullscreen = false;

        bool vsync = false;

        // 允许UI覆盖
        bool overlay = true;
    } settings;

    //鼠标的状态
    // struct {
    //     struct {
    //         bool left = false;
    //         bool middle = false;
    //         bool right = false;
    //     } buttons;
    //     glm::vec2 position;
    // } mouseState;
    //我认为使用GFLW下是是不需要使用这个的

    VkClearColorValue defaultClearColor = {0.025f, 0.025f, 0.025f, 1.0f};

    static std::vector<const char*> args; // 不确定这是干嘛的

    float timer = 0.0f;//无关帧率的计时器作用于动画

    float timerSpeed = 0.25f; //控制速率

    bool paused = false;

    Camera camera;

    std::string title = "Vulkan FrameWork";
    std::string name = "VulkanFrameWork";
    uint32_t apiVersion = VK_API_VERSION_1_3;

    //默认的renderpass使用的深度模板附件
    struct {
        VkImage image;
        VkDeviceMemory memory;
        VkImageView view;
    } depthStencil;

    using KeyCallback = std::function<void(GLFWwindow*, int, int, int, int)>;
    using MouseButtonCallback = std::function<void(GLFWwindow*, int, int, int)>;
    using CursorPosCallback = std::function<void(GLFWwindow*, double, double)>;
    using ScrollCallback = std::function<void(GLFWwindow*, double, double)>;
    //GLFW Window
    GLFWwindow* window{nullptr};
    KeyCallback userKeyCallback;
    MouseButtonCallback userMouseButtonCallback;
    CursorPosCallback userCursorPosCallback;
    ScrollCallback userScrollCallback;


    vulkanFrameWork(){};
    virtual ~vulkanFrameWork();
    //设置Vulkan的实例，设置准许的扩展和链接可用的物理设备
    bool initVulkan();

    bool setWindow();

    //设置Vulkan的基础框架
    virtual VkResult createInstance();
    virtual void render() = 0;

    virtual void setKeyCallback(KeyCallback callback);
    virtual void setMouseButtonCallback(MouseButtonCallback callback);
    virtual void setCursorPosCallback(CursorPosCallback callback);
    virtual void setScrollCallback(ScrollCallback callback);

    static void KeyCallbackInternal(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void MouseButtonCallbackInternal(GLFWwindow* window, int button, int action, int mods);
    static void CursorPosCallbackInternal(GLFWwindow* window, double xpos, double ypos);
    static void ScrollCallbackInternal(GLFWwindow* window, double xoffset, double yoffset);
    //内部转发回调的函数

    virtual void windowResized();

    virtual void buildCommandBuffers();

    virtual void setupDepthStencil();

    virtual void setupFrameBuffer();

    virtual void setupRenderPass();

    virtual void getEnabledFeatures();

    virtual void getEnabledExtensions();

    virtual void prepare();

    //加载SPIR-V文件
    VkPipelineShaderStageCreateInfo loadShader(const std::string& fileName, VkShaderStageFlagBits stage);

    void windowResize();

    void renderLoop();

    void drawUI(const VkCommandBuffer& commandBuffer);

    void prepareFrame();

    void submitFrame();

    virtual void renderFrame();

    virtual void OnUpdateUIOverlay(FrameWork::VulkanUIOverlay* overlay);
};



#endif //VULKANFRAMEWORK_H
