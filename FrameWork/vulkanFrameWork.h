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

#include "CaIMaterial.h"
#include "pubh.h"

#include "VulkanSwapChain.h"    // 确保这些头文件也不会以冲突的方式包含 vulkan.h。
#include "Camera.h"
#include "DescriptorPool.h"
#include "PublicStruct.h"
#include "Resource.h"
#include "Slot.h"
#define MAX_FRAME 2


namespace FG {
    struct TextureDescription;
}

namespace FG {
    struct BaseDescription;
}

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
    void CreateSwapChainTex();

    void createCommandBuffers();

    void destroyCommandBuffers();


    std::string shaderDir = "glsl";

    //外部服务注册
    FrameWork::InputManager &inputManager = FrameWork::InputManager::GetInstance();
    FrameWork::Resource &resourceManager = FrameWork::Resource::GetInstance();

    //动态的描述符池
    FrameWork::VulkanDescriptorPool vulkanDescriptorPool;

    using WindowResizedCallback = std::function<void()>;
    std::vector<WindowResizedCallback> windowResizedCallbacks;

    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

    std::unordered_map<std::string, uint32_t> texturePathMap;

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

    std::vector<const char *> enabledDeviceExtensions;
    std::vector<const char *> enabledInstanceExtensions;
    std::vector<VkLayerSettingEXT> enabledLayerSettings;
    //验证层的一些设置信息

    //初始值
    void *deviceCreatepNextChain = nullptr; //扩展结构
    VkDevice device{VK_NULL_HANDLE};
    VkQueue graphicsQueue{VK_NULL_HANDLE};
    VkFormat depthFormat{VK_FORMAT_UNDEFINED};
    VkCommandPool commandPool{VK_NULL_HANDLE};
    VkPipelineStageFlags submitPipelineStages{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submitInfo{};
    std::vector<VkCommandBuffer> drawCmdBuffers;
    uint32_t imageIndex{0};
    VkDescriptorPool descriptorPool{VK_NULL_HANDLE};
    std::vector<VkShaderModule> shaderModules;
    VkPipelineCache pipelineCache{VK_NULL_HANDLE};
    VulkanSwapChain swapChain;
    //呈现
    std::vector<uint32_t> swapChainTextures;//保持方便重建

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
    std::vector<FrameWork::Texture *> textures;
    std::vector<FrameWork::Mesh *> meshes;
    std::vector<FrameWork::VulkanAttachment *> attachmentBuffers;
    std::vector<FrameWork::VulkanFBO *> vulkanFBOs;
    std::vector<FrameWork::VulkanPipeline *> vulkanPipelines;
    std::vector<FrameWork::VulkanPipelineInfo *> vulkanPipelineInfos;
    std::vector<FrameWork::Material *> materials;
    std::vector<FrameWork::Model *> models;
    std::vector<FrameWork::StorageBuffer *> storageBuffers;
    std::vector<FrameWork::Slot *> slots_;
    std::vector<FrameWork::MaterialData*> materialDatas_;
    std::vector<FrameWork::VulkanModelData*> modelDatas_; //更浅
    std::vector<FrameWork::CompMaterialData*> compMaterialDatas_;

    // 对象池的锁声明
    std::mutex texturesMutex;
    std::mutex meshesMutex;
    std::mutex attachmentBuffersMutex;
    std::mutex vulkanFBOsMutex;
    std::mutex vulkanPipelinesMutex;
    std::mutex vulkanPipelineInfosMutex;
    std::mutex materialsMutex;
    std::mutex modelsMutex;
    std::mutex storageBuffersMutex;
    std::mutex slotsMutex;
    std::mutex materialDatasMutex;
    std::mutex modelDatasMutex;
    std::mutex compMaterialDatasMutex;

    std::unordered_map<std::string, VkRenderPass> renderPasses; //记录renderpass
    std::unordered_map<RenderPassType, VkRenderPass> renderPassTable;
    std::unordered_map<std::string, VkDescriptorSetLayout> descriptorSetLayouts;

    using ReleaseContainer = std::pair<uint32_t, uint32_t>; //后者是释放计数器
    std::deque<ReleaseContainer> textureReleaseQueue;
    std::deque<ReleaseContainer> meshReleaseQueue;
    std::deque<ReleaseContainer> attachmentReleaseQueue;
    std::deque<ReleaseContainer> fboReleaseQueue;
    std::deque<ReleaseContainer> pipelineReleaseQueue;
    std::deque<ReleaseContainer> materialDataReleaseQueue;
    std::deque<ReleaseContainer> modelDataReleaseQueue;
    std::deque<ReleaseContainer> compMaterialDataReleaseQueue;

    //多线程安全，上锁
    std::mutex texDeleteMutex;
    std::mutex meshDeleteMutex;
    std::mutex attachmentDeleteMutex;
    std::mutex fboDeleteMutex;
    std::mutex pipelineDeleteMutex;
    std::mutex materialDeleteMutex;
    std::mutex modelDataDeleteMutex;
    std::mutex compMaterialDeleteMutex;


    std::string title = "Vulkan FrameWork";
    std::string name = "VulkanFrameWork";

public:
    friend class FrameWork::Slot;

    uint32_t currentFrame = 0;
    uint32_t MaxFrame = MAX_FRAME;
    uint32_t windowWidth{1280};
    uint32_t windowHeight{720};
    //UI

    double frameTimer = 1.0;

    FrameWork::VulkanDevice *vulkanDevice{};

    struct Settings {
        //验证层
        bool validation = true;

        bool fullscreen = false;

        bool vsync = false;

        // 允许UI覆盖
        bool overlay = false;
    } settings;


    inline static VkClearColorValue defaultClearColor = {0.0025f, 0.0025f, 0.0025f, 1.0f};

    static std::vector<const char *> args; // 不确定这是干嘛的

    float timer = 0.0f; //无关帧率的计时器作用于动画

    float timerSpeed = 0.25f; //控制速率

    uint32_t apiVersion = VK_API_VERSION_1_3;

    //默认的renderpass使用的深度模板附件


    //GLFW Window
    GLFWwindow *window{nullptr};


    virtual ~vulkanFrameWork() = default;

    vulkanFrameWork(const vulkanFrameWork &) = delete;

    vulkanFrameWork &operator=(const vulkanFrameWork &) = delete;

    //设置Vulkan的实例，设置准许的扩展和链接可用的物理设备
    bool initVulkan();

    void DestroyAll();

    bool setWindow();

    //设置Vulkan的基础框架
    virtual VkResult createInstance();

    virtual void render();

    virtual void setupRenderPass();

    virtual void getEnabledFeatures();

    virtual void getEnabledExtensions();

    virtual void prepare();

    //加载SPIR-V文件
    VkPipelineShaderStageCreateInfo loadShader(const std::string &fileName, VkShaderStageFlagBits stage);

    void windowResize();

    // void renderLoop();

    void prepareFrame(double deltaMilliTime);

    void submitFrame();



    void CreateGPUBuffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, FrameWork::Buffer &buffer, void *data);

    FrameWork::Buffer CreateUniformBuffer(const std::vector<FrameWork::ShaderProperty>& properties);

    //将数据直接设置到GPU内存方便后续创建local 内存
    void CreateTexture(uint32_t &textureId, const FrameWork::TextureFullData &textureData);

    //这个函数是用来创建FrameGraph中的临时资源，一般作为Attachment，这种资源一般不需要array和mipmap,且是2D资源，
    //如果其他类型的资源直接作为Proxy导入为好，每一帧创建开销太大了
    void CreateTexture(uint32_t & textureId, uint32_t& resolveID, FG::BaseDescription* description);

    std::vector<uint32_t>& GetSwapChainTextures();

    void CreateImageView(FrameWork::VulkanImage &image, VkImageView &imageView, VkImageAspectFlags aspectFlags,
                         VkImageViewType viewType);

    void CreateAttachment(uint32_t &attachmentId, uint32_t width, uint32_t height, AttachmentType attachmentType,
                          VkSampleCountFlagBits numSample, bool isSampled); //最后一个参数的含义是是否会作为纹理被着色器采样
    void ReCreateAttachment();

    void CreateFrameBuffer(uint32_t &frameBufferId, const std::vector<uint32_t> &attachments, uint32_t width,
                           uint32_t height, VkRenderPass renderPass);

    void CreatePresentFrameBuffer(std::unique_ptr<FrameWork::VulkanFBO>& presentFBO, VkRenderPass renderPass);

    void RecreateFrameBuffer(uint32_t &frameBufferId);

    void RecreatePresentFrameBuffer(std::unique_ptr<FrameWork::VulkanFBO>& presentFBO);

    void RegisterRenderPass(VkRenderPass renderPass, const std::string &name);

    void RegisterDescriptorSetLayout(VkDescriptorSetLayout &descriptorSetLayout, const std::string &name);

    void UnRegisterRenderPass(const std::string &name);

    //创建pipelineInfo
    //创建管线
    void InitPipelineInfo(uint32_t &pipelineInfoIdx);

    void LoadPipelineShader(uint32_t &pipelineInfoIdx, const std::string &fileName, VkShaderStageFlagBits stage);

    void AddPipelineVertexBindingDescription(uint32_t &pipelineInfoIdx,
                                             VkVertexInputBindingDescription &bindingDescription);

    void AddPipelineVertexBindingDescription(uint32_t &pipelineInfoIdx,
                                             const std::vector<VkVertexInputBindingDescription> &bindingDescriptions);

    void AddPipelineVertexAttributeDescription(uint32_t &pipelineInfoIdx,
                                               const std::vector<VkVertexInputAttributeDescription> &
                                               attributeDescriptions);

    void SetPipelineInputAssembly(uint32_t &pipelineInfoIdx, VkPipelineInputAssemblyStateCreateInfo info);

    void SetPipelineViewPort(uint32_t &pipelineInfoIdx, const VkViewport &viewport);

    void SetPipelineScissor(uint32_t &pipelineInfoIdx, const VkRect2D &scissor);

    void SetPipelineRasterizationState(uint32_t &pipelineInfoIdx, VkPipelineRasterizationStateCreateInfo createInfo);

    void SetPipelineMultiSampleState(uint32_t &pipelineInfoIdx, VkPipelineMultisampleStateCreateInfo info);

    void SetPipelineDepthStencilState(uint32_t &pipelineInfoIdx, VkPipelineDepthStencilStateCreateInfo info);

    void AddPipelineColorBlendState(uint32_t &pipelineInfoIdx, bool hasColor, BlendOp blendOp,
                                    VkColorComponentFlags colorWriteMask =
                                            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);

    //这里注意默认UniformObject时一个大结构体，也方便管理使用偏移更新不失性能，且注意DescriptorSetLayout只需要提供两个种类，具体数量通过后面两个参数控制

    //适配CaIShader处理ShaderState
    VkPipelineColorBlendAttachmentState SetPipelineColorBlendAttachment(const FrameWork::ShaderInfo &shaderInfo);

    std::vector<VkPipelineShaderStageCreateInfo> SetPipelineShaderStageInfo(
        const FrameWork::ShaderModulePackages &shaderModules);

    VkPipelineInputAssemblyStateCreateInfo SetPipelineInputAssembly();

    VkPipelineRasterizationStateCreateInfo SetRasterization(const FrameWork::ShaderInfo &shaderInfo);

    //对于MSAA属性直接根据传入的RenderPass的Type来选择是否使用
    VkPipelineDepthStencilStateCreateInfo SetDepthStencil(const FrameWork::ShaderInfo &shaderInfo);

    //对于动态调节大小这里默认可以


    void CreateVulkanPipeline(uint32_t &pipelineIdx, const std::string &name, uint32_t &pipelineInfoIdx,
                              const std::string &renderPassName, uint32_t subpass,
                              const std::vector<VkDescriptorSetLayout> &descriptorSetLayout, uint32_t uniform,
                              uint32_t texNum); //最后一项是为了创建的pipelineLayout
    void CreateVulkanPipeline(uint32_t &pipelineIdx, const std::string &name, uint32_t &pipelineInfoIdx,
                              const std::string &renderPassName, uint32_t subpass,
                              const std::vector<VkDescriptorSetLayout> &descriptorSetLayout);

    //适配CaIShader                                                                                                                                              //这里的宽和高设置渲染的视口大小，这里的默认值为-1，作用是默认为窗口大小
    FrameWork::ShaderInfo CreateVulkanPipeline(uint32_t &pipelineIdx, const std::string &shaderPath,
                                               RenderPassType renderPassType, uint32_t subpass = 0, uint32_t width = -1,
                                               uint32_t height = -1);

    FrameWork::ShaderInfo CreateVulkanPipeline(uint32_t &pipelineIdx, const std::string &shaderPath,
                                               VkRenderPass renderPass, uint32_t subpass = 0, uint32_t width = -1,
                                               uint32_t height = -1);
    //先只支持多pass，如果支持subpass则在各种延迟渲染中需要使用InputAttachment来代替普通RenderPass使用纹理传入的Attachment需要分类讨论

    //支持Dynamic Rendering Pipeline
    FrameWork::ShaderInfo CreateVulkanPipeline(uint32_t &pipelineIdx, const std::string &shaderPath,VkFormat colorFormat = VK_FORMAT_UNDEFINED, uint32_t width = -1,
                                               uint32_t height = -1);

    //简单封装
    VkCommandBuffer BeginCommandBuffer() const;

    void BeginRenderPass(const std::string &renderPassName, uint32_t frameBufferID, uint32_t renderWidth,
                         uint32_t renderHeight, VkClearColorValue clearColorValue = defaultClearColor) const;

    void BeginRenderPass(VkRenderPass renderPass, uint32_t frameBufferID, uint32_t renderWidth,
                         uint32_t renderHeight) const;

    void EndRenderPass() const;

    void EndCommandBuffer() const;


    //描述符
    VkDescriptorSetLayout CreateDescriptorSetLayout(
        VkDescriptorType descriptorType, VkShaderStageFlags stageFlags
    );

    void CreateMaterial(uint32_t &materialIdx, const std::vector<FrameWork::TextureFullData> &texDatas);

    void CreateMaterialData(FrameWork::CaIMaterial& caiMaterial);

    void UpdateUniformBuffer(const std::vector<FrameWork::Buffer> &uniformBuffer, const std::vector<void *> &data,
                             const std::vector<uint32_t> &sizes, uint32_t offset);

    VkSampler CreateSampler(uint32_t mipmapLevels);

    void SetUpStaticMesh(unsigned int &meshID, std::vector<FrameWork::Vertex> &vertices, std::vector<uint32_t> &indices,
                         bool skinned);

    void LoadModel(uint32_t &modelID, const std::string &fileName, ModelType modelType,
                   TextureTypeFlags textureTypeFlags, glm::vec3 position = {0, 0, 1}, float scale = 1.0f);

    void LoadVulkanModel(uint32_t& modelDataID, const std::string &fileName, ModelType modelType,
                    TextureTypeFlags textureTypeFlags, glm::vec3 position = {0, 0, 1}, float scale = 1.0f);

    void BindMesh(VkCommandBuffer commandBuffer, uint32_t meshData);

    void DrawModel(uint32_t modelID, VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t firstSet);



    FrameWork::Slot *CreateSlot(uint32_t &slotID);

    void UpdateAllSlots();

    //计算着色器
    void CreateVulkanComputePipeline(uint32_t &pipelineInfoIdx, const std::string &fileName,
                                     const std::vector<VkDescriptorSetLayout> &descriptorSetLayouts); //注意这和上面的管线不同
    void CreateGPUStorgeBuffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, FrameWork::Buffer &buffer, void *data);

    void CreateHostVisibleStorageBuffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, FrameWork::Buffer &buffer,
                                        void *data);

    //快速生成基础几何模型
    void GenFace(uint32_t &model, const glm::vec3 &position, const glm::vec3 &normal, float width, float height,
                 std::string texPath = "");

    void GenFaceData(uint32_t& modelDataID, const glm::vec3 &position, const glm::vec3 &normal, float width, float height, const std::string& texPath = "");

    //参数变量向外接口
    static vulkanFrameWork &GetInstance(); //单例接口
    uint32_t GetFrameWidth() const;

    uint32_t GetFrameHeight() const;

    uint32_t GetCurrentFrame() const;

    VkRenderPass GetRenderPass(const std::string &name) const;

    FrameWork::VulkanDevice *GetVulkanDevice() const;

    void SetTitle(const std::string &title);

    VkCommandBuffer GetCurrentCommandBuffer() const;

    uint32_t GetCurrentImageIndex() const;

    const VulkanSwapChain &GetVulkanSwapChain() const;

    VkInstance &GetVulkanInstance();

    VkQueue GetVulkanGraphicsQueue() const;

    VkPhysicalDevice GetVulkanPhysicalDevice() const;

    VkFormat GetDepthFormat() const;

    VkSampleCountFlagBits GetSampleCount() const;

    //DeleteQueue实现
    // std::deque<ReleaseContainer> textureReleaseQueue;
    // std::deque<ReleaseContainer> meshReleaseQueue;
    // std::deque<ReleaseContainer> attachmentReleaseQueue;
    // std::deque<ReleaseContainer> fboReleaseQueue;
    // std::deque<ReleaseContainer> pipelineReleaseQueue;

    template<typename T>
    void processReleaseQueue(std::deque<std::pair<uint32_t, uint32_t> > &queue) {
        for (int i = queue.size() - 1; i >= 0; i--) {
            queue[i].second--;
        }
        while (! queue.empty() && queue.front().second == 0) {
            destroyByIndex<T>(queue.front().first);
            queue.pop_front();
        }
    }

    void DeleteTexture(uint32_t id);

    void DeleteMesh(uint32_t id);

    void DeleteAttachment(uint32_t id);

    void DeleteFBO(uint32_t id);

    void DeletePipeline(uint32_t id);

    void DeleteMaterialData(uint32_t id);

    void DeleteModelData(uint32_t id);

    void DeleteCompMaterialData(uint32_t id);

    void CheckDelete(); //每帧进行清理资源,在帧尾清理


    void SetWindowResizedCallBack(const WindowResizedCallback &callback);

    // 封装对象的池
    template<class T>
    decltype(auto) getVectorRef() {
        if constexpr (std::is_same_v<T, FrameWork::Texture>) {
            return reinterpret_cast<std::vector<T *> &>(textures);
        } else if constexpr (std::is_same_v<T, FrameWork::Mesh>) {
            return reinterpret_cast<std::vector<T *> &>(meshes);
        } else if constexpr (std::is_same_v<T, FrameWork::VulkanAttachment>) {
            return reinterpret_cast<std::vector<T *> &>(attachmentBuffers);
        } else if constexpr (std::is_same_v<T, FrameWork::VulkanFBO>) {
            return reinterpret_cast<std::vector<T *> &>(vulkanFBOs);
        } else if constexpr (std::is_same_v<T, FrameWork::VulkanPipelineInfo>) {
            return reinterpret_cast<std::vector<T *> &>(vulkanPipelineInfos);
        } else if constexpr (std::is_same_v<T, FrameWork::VulkanPipeline>) {
            return reinterpret_cast<std::vector<T *> &>(vulkanPipelines);
        } else if constexpr (std::is_same_v<T, FrameWork::Material>) {
            return reinterpret_cast<std::vector<T *> &>(materials);
        } else if constexpr (std::is_same_v<T, FrameWork::Model>) {
            return reinterpret_cast<std::vector<T *> &>(models);
        } else if constexpr (std::is_same_v<T, FrameWork::StorageBuffer>) {
            return reinterpret_cast<std::vector<T *> &>(storageBuffers);
        } else if constexpr (std::is_same_v<T, FrameWork::Slot>) {
            return reinterpret_cast<std::vector<T *> &>(slots_);
        } else if constexpr (std::is_same_v<T, FrameWork::MaterialData>) {
            return reinterpret_cast<std::vector<T*>& >(materialDatas_);
        } else if constexpr (std::is_same_v<T, FrameWork::VulkanModelData>) {
            return reinterpret_cast<std::vector<T*>&> (modelDatas_);
        } else if constexpr (std::is_same_v<T, FrameWork::CompMaterialData>) {
            return reinterpret_cast<std::vector<T*>&> (compMaterialDatas_);
        }
        else {
            std::cerr << "Unknown type in getNextIndex!" << std::endl;
            exit(-1);
        }
    }

    template<class T>
    std::mutex& getMutex() {
        if constexpr (std::is_same_v<T, FrameWork::Texture>) {
            return texturesMutex;
        } else if constexpr (std::is_same_v<T, FrameWork::Mesh>) {
            return meshesMutex;
        } else if constexpr (std::is_same_v<T, FrameWork::VulkanAttachment>) {
            return attachmentBuffersMutex;
        } else if constexpr (std::is_same_v<T, FrameWork::VulkanFBO>) {
            return vulkanFBOsMutex;
        } else if constexpr (std::is_same_v<T, FrameWork::VulkanPipelineInfo>) {
            return vulkanPipelineInfosMutex;
        } else if constexpr (std::is_same_v<T, FrameWork::VulkanPipeline>) {
            return vulkanPipelinesMutex;
        } else if constexpr (std::is_same_v<T, FrameWork::Material>) {
            return materialsMutex;
        } else if constexpr (std::is_same_v<T, FrameWork::Model>) {
            return modelsMutex;
        } else if constexpr (std::is_same_v<T, FrameWork::StorageBuffer>) {
            return storageBuffersMutex;
        } else if constexpr (std::is_same_v<T, FrameWork::Slot>) {
            return slotsMutex;
        } else if constexpr (std::is_same_v<T, FrameWork::MaterialData>) {
            return materialDatasMutex;
        } else if constexpr (std::is_same_v<T, FrameWork::VulkanModelData>) {
            return modelDatasMutex;
        } else if constexpr (std::is_same_v<T, FrameWork::CompMaterialData>) {
            return compMaterialDatasMutex;
        }
        else {
            static std::mutex dummy_mutex;
            return dummy_mutex;
        }
    }


    template<class T>
    uint32_t inline getNextIndex() {
        std::lock_guard<std::mutex> lock(getMutex<T>());
        auto &vec = getVectorRef<T>();
        auto len = vec.size();
        for (uint32_t i = 0; i < vec.size(); i++) {
            if (vec[i] == nullptr) return i;
            if (vec[i]->inUse == false) {
                return i;
            }
        }
        vec.push_back(new T());
        return static_cast<uint32_t>(len);
    }

    template<class T>
    auto inline getByIndex(uint32_t index) -> T * {
        std::lock_guard<std::mutex> lock(getMutex<T>());
        return getVectorRef<T>()[index];
    }

    template<typename T>
    auto getSize() -> uint32_t {
        std::lock_guard<std::mutex> lock(getMutex<T>());
        return getVectorRef<T>().size();
    }

    template<class T>
    void inline destroyByIndex(uint32_t index) {
        auto obj = getByIndex<T>(index);//这里已经上锁
        if (obj == nullptr || obj->inUse == false) {
            return;
        }
        if (std::is_same_v<T, FrameWork::Texture>) {
            auto texture = textures[index];
            texture->inUse = false;
            if (texture->isSwapChainRef == true) {
                return;
            }
            if (texture->imageView != VK_NULL_HANDLE) {
                vkDestroyImageView(device, texture->imageView, nullptr);
                texture->imageView = VK_NULL_HANDLE;
            }

            texture->image.destroy();

            if (texture->sampler != VK_NULL_HANDLE) {
                vkDestroySampler(device, texture->sampler, nullptr);
                texture->sampler = VK_NULL_HANDLE;
            }
        } else if (std::is_same_v<T, FrameWork::Mesh>) {
            auto mesh = meshes[index];
            mesh->inUse = false;
            mesh->vertexCount = 0;
            mesh->indexCount = 0;
            mesh->VertexBuffer.destroy();
            mesh->IndexBuffer.destroy();
        } else if (std::is_same_v<T, FrameWork::VulkanAttachment>) {
            auto attachment = attachmentBuffers[index];
            attachment->inUse = false;

            for (auto &t: attachment->attachmentsArray) {
                destroyByIndex<FrameWork::Texture>(t);
            }
        } else if (std::is_same_v<T, FrameWork::VulkanFBO>) {
            auto vulkanFBO = vulkanFBOs[index];
            vulkanFBO->inUse = false;
            for (auto &t: vulkanFBO->framebuffers) {
                vkDestroyFramebuffer(device, t, nullptr);
            }
            for (auto& i : vulkanFBO->AttachmentsIdx) {
                destroyByIndex<FrameWork::VulkanAttachment>(i);
            }
        } else if (std::is_same_v<T, FrameWork::VulkanPipelineInfo>) {
            auto vulkanPipelineInfo = vulkanPipelineInfos[index];
            vulkanPipelineInfo->inUse = false;
        } else if (std::is_same_v<T, FrameWork::VulkanPipeline>) {
            auto vulkanPipeline = vulkanPipelines[index];
            vulkanPipeline->inUse = false;
            if (vulkanPipeline->pipelineLayout != VK_NULL_HANDLE) {
                vkDestroyPipelineLayout(device, vulkanPipeline->pipelineLayout, nullptr);
            }
            if (vulkanPipeline->pipeline != VK_NULL_HANDLE) {
                vkDestroyPipeline(device, vulkanPipeline->pipeline, nullptr);
            }
        } else if (std::is_same_v<T, FrameWork::Material>) {
            auto material = materials[index];
            material->inUse = false;
            for (auto &tex: material->textures) {
                destroyByIndex<FrameWork::Texture>(tex);
            }
        } else if (std::is_same_v<T, FrameWork::Model>) {
            auto model = models[index];
            model->inUse = false;
            for (auto &m: model->materials) {
                destroyByIndex<FrameWork::Material>(m);
            }
            for (auto &mesh: model->meshes) {
                destroyByIndex<FrameWork::Mesh>(mesh);
            }
        } else if (std::is_same_v<T, FrameWork::StorageBuffer>) {
            auto storageBuffer = storageBuffers[index];
            storageBuffer->inUse = false;
            storageBuffer->buffer.destroy();
            storageBuffer->itemNum = 0;
        } else if (std::is_same_v<T, FrameWork::Slot>) {
            auto &slot = slots_[index];
            slot->inUse = false;
            delete slot;
            slot = nullptr;
        } else if (std::is_same_v<T, FrameWork::MaterialData>) {
            auto & materialData = materialDatas_[index];
            materialData->inUse = false;
            for (int i = 0; i < materialData->descriptorSetLayouts.size(); ++i) {
                vulkanDescriptorPool.RegisterUnusedDescriptorSet(materialData->descriptorSetLayouts[i],
                    materialData->descriptorSets[i]);
            }
            for (auto& buffer : materialData->vertexUniformBuffers) {
                buffer.destroy();
            }
            for (auto& buffer : materialData->fragmentUniformBuffers) {
                buffer.destroy();
            }
        } else if (std::is_same_v<T, FrameWork::VulkanModelData>) {
            auto modelData = getByIndex<FrameWork::VulkanModelData>(index);
            modelData->inUse = false;

            for (auto& textureIDarray : modelData->textures) {
                for(auto& [_, textureID] : textureIDarray)
                    destroyByIndex<FrameWork::Texture>(textureID);
            }
            for (auto& meshID : modelData->meshIDs) {
                destroyByIndex<FrameWork::Mesh>(meshID);
            }
        } else if (std::is_same_v<T, FrameWork::CompMaterialData>) {
            auto compMaterialData = getByIndex<FrameWork::CompMaterialData>(index);
            compMaterialData->inUse = false;

            for (int i = 0; i < compMaterialData->descriptorSetLayouts.size(); ++i) {
                vulkanDescriptorPool.RegisterUnusedDescriptorSet(compMaterialData->descriptorSetLayouts[i],
                    compMaterialData->descriptorSets[i]);
            }

            for (auto& uniformBuffer : compMaterialData->uniformBuffers) {
                uniformBuffer.destroy();
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
