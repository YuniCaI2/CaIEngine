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

#include "pubh.h"

#include "VulkanSwapChain.h"    // 确保这些头文件也不会以冲突的方式包含 vulkan.h。
#include "Camera.h"
#include "DescriptorPool.h"
#include "PublicStruct.h"
#include "Resource.h"
#define MAX_FRAME 2



class VulkanSwapChain;
class vulkanFrameWork {
private:
    vulkanFrameWork();
    std::string getWindowTitle() const; //窗口标题
    // void nextFrame();
    void createPipelineCache();
    void createSynchronizationPrimitives(); //创建一些同步图元的对象
    void createSurface();
    void createSwapChain();
    void createCommandBuffers();
    void destroyCommandBuffers();

    void RecreateAllWindowFrameBuffers(); //重建所有大小和窗口一样大的帧缓冲以便显示

    std::string shaderDir = "glsl";

    //外部服务注册
    FrameWork::InputManager& inputManager = FrameWork::InputManager::GetInstance();
    FrameWork::Resource& resourceManager = FrameWork::Resource::GetInstance();

    //动态的描述符池
    FrameWork::VulkanDescriptorPool vulkanDescriptorPool;

    using WindowResizedCallback = std::function<void()>;
    WindowResizedCallback windowResizedCallback{nullptr};

    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

protected:
    //得到一个绝对路径
    std::string getShaderPath() const;


    //Frame counter to display fps
    uint32_t frameCounter = 0;
    uint32_t lastFPS = 0;
    double frameCountTimeStamp, tPrevEnd;
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

    //初始值
    void* deviceCreatepNextChain = nullptr; //扩展结构
    VkDevice device{VK_NULL_HANDLE};
    VkQueue graphicsQueue{VK_NULL_HANDLE};
    VkFormat depthFormat{VK_FORMAT_UNDEFINED};
    VkCommandPool commandPool{VK_NULL_HANDLE};
    VkPipelineStageFlags submitPipelineStages{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submitInfo{};
    std::vector<VkCommandBuffer> drawCmdBuffers;
    uint32_t imageIndex{0};
    VkDescriptorPool descriptorPool{ VK_NULL_HANDLE };
    std::vector<VkShaderModule> shaderModules;
    VkPipelineCache pipelineCache{ VK_NULL_HANDLE };
    VulkanSwapChain swapChain;
    //呈现
    uint32_t presentFrameBufferIndex{0};
    uint32_t presentPipelineIndex{0};
    uint32_t presentMaterialIndex{0};
    FrameWork::MaterialCreateInfo presentMaterialCreateInfo{}; //用来记录重建信息

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
    std::vector<FrameWork::VulkanPipeline*> vulkanPipelines;
    std::vector<FrameWork::VulkanPipelineInfo*> vulkanPipelineInfos;
    std::unordered_map<std::string, VkRenderPass> renderPasses;//记录renderpass
    std::unordered_map<std::string, VkDescriptorSetLayout> descriptorSetLayouts;
    std::vector<FrameWork::Material*> materials;
    std::vector<FrameWork::Model*> models;

    std::string title = "Vulkan FrameWork";
    std::string name = "VulkanFrameWork";

public:
    uint32_t currentFrame = 0;
    uint32_t MaxFrame = MAX_FRAME;
    uint32_t windowWidth{1280};
    uint32_t windowHeight{720};
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

    uint32_t apiVersion = VK_API_VERSION_1_3;

    //默认的renderpass使用的深度模板附件
    struct {
        VkImage image;
        VkDeviceMemory memory;
        VkImageView view;
    } depthStencil;

    //GLFW Window
    GLFWwindow* window{nullptr};



    virtual ~vulkanFrameWork();
    //设置Vulkan的实例，设置准许的扩展和链接可用的物理设备
    bool initVulkan();

    bool setWindow();

    //设置Vulkan的基础框架
    virtual VkResult createInstance();
    virtual void render();
    virtual void setupDepthStencil();
    virtual void setupRenderPass();
    virtual void getEnabledFeatures();
    virtual void getEnabledExtensions();
    virtual void prepare();

    //加载SPIR-V文件
    VkPipelineShaderStageCreateInfo loadShader(const std::string& fileName, VkShaderStageFlagBits stage);

    void windowResize();

    // void renderLoop();

    void prepareFrame(double deltaMilliTime);
    void submitFrame();
    void finishRender();


    void CreateGPUBuffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, FrameWork::Buffer& buffer, void* data);//将数据直接设置到GPU内存方便后续创建local 内存
    void CreateTexture(uint32_t& textureId, FrameWork::TextureFullData& textureData);
    void CreateImageView(FrameWork::VulkanImage& image, VkImageView& imageView, VkImageAspectFlags aspectFlags, VkImageViewType viewType);
    void CreateAttachment(uint32_t& attachmentId, uint32_t width, uint32_t height, AttachmentType attachmentType, VkSampleCountFlagBits numSample, bool isSampled); //最后一个参数的含义是是否会作为纹理被着色器采样
    void CreateFrameBuffer(uint32_t& frameBufferId, const std::vector<uint32_t>& attachments, uint32_t width, uint32_t height, VkRenderPass renderPass);
    void CreatePresentFrameBuffer(uint32_t& frameBufferId, uint32_t attachment, VkRenderPass renderPass);
    void RegisterRenderPass(VkRenderPass renderPass, const std::string& name);
    void RegisterDescriptorSetLayout(VkDescriptorSetLayout& descriptorSetLayout, const std::string& name);
    void UnRegisterRenderPass(const std::string& name);

    //创建pipelineInfo
    //创建管线
    void InitPipelineInfo(uint32_t& pipelineInfoIdx);
    void LoadPipelineShader(uint32_t& pipelineInfoIdx, const std::string& fileName, VkShaderStageFlagBits stage);
    void AddPipelineVertexBindingDescription(uint32_t& pipelineInfoIdx, VkVertexInputBindingDescription& bindingDescription);
    void AddPipelineVertexBindingDescription(uint32_t& pipelineInfoIdx, const std::vector<VkVertexInputBindingDescription>& bindingDescriptions);
    void AddPipelineVertexAttributeDescription(uint32_t& pipelineInfoIdx, const std::vector<VkVertexInputAttributeDescription>& attributeDescriptions);
    void SetPipelineInputAssembly(uint32_t& pipelineInfoIdx,VkPipelineInputAssemblyStateCreateInfo info);
    void SetPipelineViewPort(uint32_t& pipelineInfoIdx, const VkViewport& viewport);
    void SetPipelineScissor(uint32_t& pipelineInfoIdx, const VkRect2D& scissor);
    void SetPipelineRasterizationState(uint32_t& pipelineInfoIdx, VkPipelineRasterizationStateCreateInfo createInfo);
    void SetPipelineMultiSampleState(uint32_t& pipelineInfoIdx, VkPipelineMultisampleStateCreateInfo info);
    void SetPipelineDepthStencilState(uint32_t& pipelineInfoIdx, VkPipelineDepthStencilStateCreateInfo info);
    void AddPipelineColorBlendState(uint32_t& pipelineInfoIdx, bool hasColor, BlendOp blendOp,
        VkColorComponentFlags colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);
    //这里注意默认UniformObject时一个大结构体，也方便管理使用偏移更新不失性能，且注意DescriptorSetLayout只需要提供两个种类，具体数量通过后面两个参数控制
    void CreateVulkanPipeline(uint32_t& pipelineIdx, const std::string& name, uint32_t& pipelineInfoIdx, const std::string& renderPassName, uint32_t subpass, const std::vector<VkDescriptorSetLayout>& descriptorSetLayout, uint32_t uniform, uint32_t texNum);//最后一项是为了创建的pipelineLayout

    //简单封装
    VkCommandBuffer BeginCommandBuffer() const;
    void BeginRenderPass(const std::string& renderPassName, uint32_t frameBufferID, uint32_t renderWidth, uint32_t renderHeight) const;
    void BeginRenderPass(VkRenderPass renderPass, uint32_t frameBufferID, uint32_t renderWidth, uint32_t renderHeight) const;
    void EndRenderPass() const;
    void EndCommandBuffer() const;


    //初始化呈现
    void InitPresent(const std::string& presentShaderName, uint32_t colorAttachmentID);
    void PresentFrame(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    //描述符
    VkDescriptorSetLayout CreateDescriptorSetLayout(
        VkDescriptorType descriptorType, VkShaderStageFlags stageFlags
        );
    void CreateMaterial(uint32_t& materialIdx, FrameWork::MaterialCreateInfo& materialInfo);
    void UpdateUniformBuffer(const std::vector<FrameWork::Buffer>& uniformBuffer, const std::vector<void*>& data, const std::vector<uint32_t>& sizes, uint32_t offset);
    VkSampler CreateSampler(uint32_t mipmapLevels);
    void SetUpStaticMesh(unsigned int& meshID, std::vector<FrameWork::Vertex>& vertices, std::vector<uint32_t>& indices, bool skinned);
    void LoadModel(uint32_t& modelID, const std::string& fileName, ModelType modelType, FrameWork::MaterialCreateInfo materialInfo, TextureTypeFlags textureTypeFlags, glm::vec3 position = {0, 0, 0});
    void DrawModel(uint32_t modelID, VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);
    void DrawMesh(uint32_t meshID, VkCommandBuffer commandBuffer);
    void BindMaterial(uint32_t materialID, VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);

    //计算着色器
    void CreateVulkanComputePipeline(uint32_t& pipelineInfoIdx, const std::string& fileName, const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts); //注意这和上面的管线不同
    void CreateGPUStorgeBuffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, FrameWork::Buffer& buffer, void* data);
    void CreateHostVisibleStorageBuffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, FrameWork::Buffer& buffer, void* data);

    //参数变量向外接口
    static vulkanFrameWork& GetInstance(); //单例接口
    uint32_t GetFrameWidth() const;
    uint32_t GetFrameHeight() const;
    uint32_t GetCurrentFrame() const;
    VkRenderPass GetRenderPass(const std::string& name) const;
    FrameWork::VulkanDevice* GetVulkanDevice() const;
    void SetTitle(const std::string& title);
    VkCommandBuffer GetCurrentCommandBuffer() const;
    uint32_t GetCurrentImageIndex() const;
    const VulkanSwapChain& GetVulkanSwapChain() const;
    VkInstance& GetVulkanInstance();
    VkQueue GetVulkanGraphicsQueue() const;
    VkPhysicalDevice GetVulkanPhysicalDevice() const;
    VkFormat GetDepthFormat() const;
    VkSampleCountFlagBits GetSampleCount() const;

    void SetWindowResizedCallBack(const WindowResizedCallback& callback);

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
        else if constexpr (std::is_same_v<T, FrameWork::VulkanPipelineInfo>) {
            return reinterpret_cast<std::vector<T*>&>(vulkanPipelineInfos);
        }
        else if constexpr (std::is_same_v<T, FrameWork::VulkanPipeline>) {
            return reinterpret_cast<std::vector<T*>&>(vulkanPipelines);
        }
        else if constexpr (std::is_same_v<T, FrameWork::Material>) {
            return reinterpret_cast<std::vector<T*>&>(materials);
        }
        else if constexpr (std::is_same_v<T, FrameWork::Model>) {
            return reinterpret_cast<std::vector<T*>&>(models);
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

    template<typename T>
    auto getSize()-> uint32_t {
        return getVectorRef<T>().size();
    }

    template<class T>
    void inline destroyByIndex(uint32_t index) {
        auto obj = getByIndex<T>(index);
        if (obj == nullptr || obj->inUse == false) {
            return;
        }
        if (std::is_same_v<T, FrameWork::Texture>) {
            auto texture = textures[index];
            texture->inUse = false;
            if (texture->imageView != VK_NULL_HANDLE) {
                vkDestroyImageView(device, texture->imageView, nullptr);
                texture->imageView = VK_NULL_HANDLE;
            }

            texture->image.destroy();

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
            if (attachment->type == AttachmentType::Present) {
                for (auto& t : attachment->attachmentsArray) {
                    auto tex = getByIndex<FrameWork::Texture>(t);
                    tex->inUse = false;
                    //保证在删除texture的时候没有删除它
                }
                return;  // 完全不处理Present类型
            }

            for (auto& t : attachment->attachmentsArray) {
                destroyByIndex<FrameWork::Texture>(t);
            }
            attachment->attachmentsArray.clear();
            attachment->attachmentsArray.resize(MAX_FRAME);
        }else if (std::is_same_v<T, FrameWork::VulkanFBO>) {
            auto vulkanFBO = vulkanFBOs[index];
            vulkanFBO->inUse = false;
            for (auto& t : vulkanFBO->framebuffers) {
                vkDestroyFramebuffer(device, t, nullptr);
            }
            for (auto& i : vulkanFBO->AttachmentsIdx) {
                destroyByIndex<FrameWork::VulkanAttachment>(i);
            }
        }else if (std::is_same_v<T, FrameWork::VulkanPipelineInfo>) {
            auto vulkanPipelineInfo = vulkanPipelineInfos[index];
            vulkanPipelineInfo->inUse = false;

        }else if (std::is_same_v<T, FrameWork::VulkanPipeline>) {
            auto vulkanPipeline = vulkanPipelines[index];
            vulkanPipeline->inUse = false;
            if (vulkanPipeline->pipeline != VK_NULL_HANDLE) {
                vkDestroyPipeline(device, vulkanPipeline->pipeline, nullptr);
            }
            if (vulkanPipeline->pipelineLayout != VK_NULL_HANDLE) {
                vkDestroyPipelineLayout(device, vulkanPipeline->pipelineLayout, nullptr);
            }
        }else if (std::is_same_v<T, FrameWork::Material>) {
            auto material = materials[index];
            material->inUse = false;
            for (auto& vertB : material->uniformBuffer) {
                vertB.destroy();
            }
            material->uniformBuffer.clear();
            for (auto& tex: material->textures) {
                destroyByIndex<FrameWork::Texture>(tex);
            }
            material->textures.clear();
            for (auto& set : material->descriptorPairs) {
                vulkanDescriptorPool.RegisterUnusedDescriptorSet(set.second, set.first);
                //注册未使用的Set
            }

            // vkFreeDescriptorSets(device, vulkanDescriptorPool.GetDescriptorPool(), material->descriptorSets.size(),
            //     material->descriptorSets.data());
            //不释放，而是使用的时候进行重写对应的DescriptorSet
        }else if (std::is_same_v<T, FrameWork::Model>) {
            auto model = models[index];
            model->inUse = false;
            for (auto& m : model->materials) {
                destroyByIndex<FrameWork::Material>(m);
            }
            for (auto& mesh : model->meshes) {
                destroyByIndex<FrameWork::Mesh>(mesh);
            }
        }
    }

};

//代替繁琐调用

#define vulkanRenderAPI vulkanFrameWork::GetInstance()

#define WINDOW_LOOP(f)          \
{                   \
while (!glfwWindowShouldClose(FrameWork::VulkanWindow::GetInstance().GetWindow())) {            \
f           \
if (FrameWork::InputManager::GetInstance().GetKey(Key_Escape)) { \
glfwSetWindowShouldClose(FrameWork::VulkanWindow::GetInstance().GetWindow(), GLFW_TRUE);     \
}                                                   \
if (vulkanFrameWork::GetInstance().GetVulkanDevice()->logicalDevice != VK_NULL_HANDLE) {                     \
vkDeviceWaitIdle(vulkanFrameWork::GetInstance().GetVulkanDevice()->logicalDevice); \
}               \
}           \
}




#endif //VULKANFRAMEWORK_H
