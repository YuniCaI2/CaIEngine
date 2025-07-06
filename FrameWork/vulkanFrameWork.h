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



// --- Vulkan 配置和头文件 ---
// 1. 定义 VK_NO_PROTOTYPES 来阻止 Vulkan SDK 定义函数符号。
#define VK_NO_PROTOTYPES

// 2. 定义 GLFW_INCLUDE_VULKAN 让 GLFW 为我们包含 <vulkan/vulkan.h>。

#include "pubh.h"

#include "VulkanSwapChain.h"    // 确保这些头文件也不会以冲突的方式包含 vulkan.h。
#include "VulkanUIOverlay.h"    // 它们应该依赖此处的设置或 volk。
#include "Camera.h"
#include "PublicStruct.h"
#include "Resource.h"
#define MAX_FRAME 2



class VulkanSwapChain;
class vulkanFrameWork {
private:
    std::string getWindowTitle() const; //窗口标题
    uint32_t destWidth{};//宽度
    uint32_t destHeight{};//高度
    bool resizing = false;//是否能调整大小
    void nextFrame();
    void createPipelineCache();
    void createCommandPool();
    void createSynchronizationPrimitives(); //创建一些同步图元的对象
    void createSurface();
    void createSwapChain();
    void createCommandBuffers();
    void destroyCommandBuffers();
    std::string shaderDir = "glsl";
    std::shared_ptr<FrameWork::InputManager> inputManager;
    std::shared_ptr<FrameWork::Resource> resourceManager;


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
    VkQueue graphicsQueue{VK_NULL_HANDLE};
    VkFormat depthFormat{VK_FORMAT_UNDEFINED};
    VkCommandPool commandPool{VK_NULL_HANDLE};
    VkPipelineStageFlags submitPipelineStages{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submitInfo{};
    std::vector<VkCommandBuffer> drawCmdBuffers;
    VkRenderPass renderPass{ VK_NULL_HANDLE };
    std::vector<VkFramebuffer>frameBuffers;
    uint32_t imageIndex{0};
    VkDescriptorPool descriptorPool{ VK_NULL_HANDLE };
    std::vector<VkShaderModule> shaderModules;
    VkPipelineCache pipelineCache{ VK_NULL_HANDLE };
    VulkanSwapChain swapChain;

    //同步信号量
    struct Semaphores {
        //SwapChain Image Presentation
        std::vector<VkSemaphore> presentComplete;
        //Command Buffer submission and execution
        std::vector<VkSemaphore> renderComplete;
        Semaphores(): presentComplete(MAX_FRAME), renderComplete(MAX_FRAME) {
        }

    } semaphores{};

    std::vector<VkFence> waitFences;
    bool requiresStencil{false};

    //各种池
    std::vector<FrameWork::Texture*> textures;
    std::vector<FrameWork::Mesh*> meshes;
    std::vector<FrameWork::VulkanAttachment*> attachmentBuffers;
    std::vector<FrameWork::VulkanFBO*> vulkanFBOs;

public:
    uint32_t currentFrame = 0;
    uint32_t MaxFrame = MAX_FRAME;
    uint32_t width{1280};
    uint32_t height{720};
    //UI

    double frameTimer = 1.0;

    FrameWork::VulkanDevice* vulkanDevice{};

    struct Settings {
        //验证层
        bool validation = false;

        bool fullscreen = false;

        bool vsync = false;

        // 允许UI覆盖
        bool overlay = false;
    } settings;


    VkClearColorValue defaultClearColor = {0.025f, 0.025f, 0.025f, 1.0f};

    static std::vector<const char*> args; // 不确定这是干嘛的

    float timer = 0.0f;//无关帧率的计时器作用于动画

    float timerSpeed = 0.25f; //控制速率

    bool paused = false;

    FrameWork::Camera camera;

    std::string title = "Vulkan FrameWork";
    std::string name = "VulkanFrameWork";
    uint32_t apiVersion = VK_API_VERSION_1_3;

    //默认的renderpass使用的深度模板附件
    struct {
        VkImage image;
        VkDeviceMemory memory;
        VkImageView view;
    } depthStencil;

    //GLFW Window
    GLFWwindow* window{nullptr};



    vulkanFrameWork(){};
    virtual ~vulkanFrameWork();
    //设置Vulkan的实例，设置准许的扩展和链接可用的物理设备
    bool initVulkan();

    bool setWindow();

    //设置Vulkan的基础框架
    virtual VkResult createInstance();
    virtual void render();

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

    void prepareFrame();
    void submitFrame();
    void finishRender();


    void CreateGPUBuffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, FrameWork::Buffer& buffer, void* data);//将数据直接设置到GPU内存方便后续创建local 内存
    void CreateTexture(uint32_t& textureId, FrameWork::TextureFullData& textureData);
    void CreateImageView(FrameWork::VulkanImage& image, VkImageView& imageView, VkImageAspectFlags aspectFlags, VkImageViewType viewType);
    VkSampler CreateSampler(uint32_t mipmapLevels);
    void SetUpStaticMesh(unsigned int& meshID, std::vector<FrameWork::Vertex>& vertices, std::vector<uint32_t>& indices, bool skinned);

    // 封装对象的池
    template<class T>
    decltype(auto) getVectorRef() {
        if constexpr (std::is_same_v<T, FrameWork::Texture>) {
            return reinterpret_cast<std::vector<T*>&>(textures);
        }
        else if constexpr (std::is_same_v<T, FrameWork::Mesh>) {
            return reinterpret_cast<std::vector<T*>&>(meshes);
        }
        else if constexpr (std::is_same_v<T, FrameWork::VulkanAttachment>) {
            return reinterpret_cast<std::vector<T*>&>(attachmentBuffers);
        }
        else if constexpr (std::is_same_v<T, FrameWork::VulkanFBO>) {
            return reinterpret_cast<std::vector<T*>&>(vulkanFBOs);
        }
        else {
            std::cerr << "Unknown type in getNextIndex!" << std::endl;
            exit(-1);
        }
    }

    template<class T>
    uint32_t inline getNextIndex() {
        auto& vec = getVectorRef<T>();
        auto len = vec.size();
        for (uint32_t i = 0; i < vec.size(); i++) {
            if (vec[i]->inUse == false) {
                return i;
            }
        }
        vec.push_back(new T());
        return static_cast<uint32_t>(len);
    }

    template<class T>
    auto inline getByIndex(uint32_t index) -> T* {
        return getVectorRef<T>()[index];
    }

    template<class T>
    void inline destroyByIndex(uint32_t index) {
        decltype(auto) vec = getVectorRef<T>();
        if (std::is_same_v<T, FrameWork::Texture>) {
            auto texture = textures[index];
            texture->inUse = false;
            vkDestroyImageView(device, texture->imageView, nullptr);
            texture->image.destroy();
            texture->imageView = VK_NULL_HANDLE;
            if (texture->sampler != VK_NULL_HANDLE) {
                vkDestroySampler(device, texture->sampler, nullptr);
                texture->sampler = VK_NULL_HANDLE;
            }
        }
        else if (std::is_same_v<T, FrameWork::Mesh>) {
            auto mesh = meshes[index];
            mesh->inUse = false;
            mesh->vertexCount = 0;
            mesh->indexCount = 0;
            mesh->inUse = false;
            mesh->VertexBuffer.destroy();
            mesh->IndexBuffer.destroy();
        }
        else if (std::is_same_v<T, FrameWork::VulkanAttachment>) {
            auto attachment = attachmentBuffers[index];
            attachment->inUse = false;
            for (auto& t : attachment->attachmentsArray) {
                destroyByIndex<FrameWork::Texture>(t);
            }
            attachment->attachmentsArray.clear();
            attachment->attachmentsArray.resize(MAX_FRAME);
        }


    }

};



#endif //VULKANFRAMEWORK_H
