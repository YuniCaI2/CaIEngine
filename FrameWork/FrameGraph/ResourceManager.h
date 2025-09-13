//
// Created by 51092 on 2025/9/10.
//

#ifndef CAIENGINE_RESOURCEMANAGER_H
#define CAIENGINE_RESOURCEMANAGER_H
#include<string>
#include"../vulkanFrameWork.h"
#include<concepts>
#include <utility>

namespace FG {
    enum class ResourceType {
        Texture,
        Buffer,
        Proxy
    };

    enum class ProxyType {
        SwapChain,
        ImportTexture,
        ImportBuffer,
        PersistentBuffer //跨帧资源
    };

    // 基础描述接口
    struct BaseDescription {
        virtual ~BaseDescription() = default;

        virtual ResourceType GetResourceType() const = 0;
    };
    template<class T>
    concept BasedDescription = std::derived_from<T, BaseDescription>;

    // 纹理描述
    struct TextureDescription : public BaseDescription {
        TextureDescription() = default;
        TextureDescription(
            uint32_t width_, uint32_t height_, VkFormat format_, uint32_t mipLevels, uint32_t arraySize,
            VkSampleCountFlagBits samples_, VkImageUsageFlags usage_
            ) : width(width_), height(height_), format(format_), mipLevels(mipLevels), arrayLayers(arraySize),samples(samples_), usages(usage_) {}
        uint32_t width{};
        uint32_t height{};
        VkFormat format{};
        uint32_t mipLevels{};
        uint32_t arrayLayers{};
        VkSampleCountFlagBits samples{};
        VkImageUsageFlags usages{};

        ResourceType GetResourceType() const override {
            return ResourceType::Texture;
        }
    };

    // 缓冲区描述
    struct BufferDescription : public BaseDescription {
        BufferDescription() = default;
        BufferDescription(
            uint32_t size_, uint32_t usages_
            ) : size(size_), usages(usages_) {}
        uint32_t size{};
        VkBufferUsageFlags usages{};
        ResourceType GetResourceType() const override {
            return ResourceType::Buffer;
        }
    };

    struct ProxyDescription : public BaseDescription {
        ProxyDescription() = default;
        uint32_t vulkanIndex{};
        ProxyType proxyType{};
        ResourceType GetResourceType() const override {
            return ResourceType::Proxy;
        }
    };

    class ResourceDescription {
    public:
        ResourceDescription() = default;

        virtual ~ResourceDescription() = default;

        ResourceDescription& SetVulkanIndex(uint32_t index_) { vulkanResourceIndex = index_; return *this; }
        ResourceDescription& SetName(const std::string &name_) { name = name_; return *this; }
        ResourceDescription& SetOutputRenderPass(uint32_t renderPass) {
            if (inputRenderPasses.contains(renderPass)) {
                LOG_ERROR("Input Render Pass : \" {} \" already exists! But you want to insert in output RenderPass !", renderPass);
            }
            outputRenderPasses.insert(renderPass); return *this;
        }
        ResourceDescription& SetInputRenderPass(uint32_t renderPass) {
            if (outputRenderPasses.contains(renderPass)) {
                LOG_ERROR("Output Render Pass : \" {} \" already exists! But you want to insert in input RenderPass !", renderPass);
            }
            inputRenderPasses.insert(renderPass); return *this;
        }

        ResourceType GetType() const { return resourceType; }
        uint32_t GetVulkanIndex() const { return vulkanResourceIndex; }
        std::string GetName() const { return name; }
        std::unordered_set<uint32_t> GetOutputRenderPass() const { return outputRenderPasses; }
        std::unordered_set<uint32_t> GetInputRenderPass() const { return inputRenderPasses; }

        // 直接存储和访问具体类型
        template<typename T>
        requires BasedDescription<T>
        void SetDescription(std::unique_ptr<BaseDescription>&& baseDescription) {
            descriptorTypeID =  VulkanTool::IndexGetter<ResourceDescription>::Get<T>();
            description = std::move(baseDescription);
            resourceType = description->GetResourceType();
        }

        template<typename T>
        requires BasedDescription<T>
        T* GetDescription() {
           if ( descriptorTypeID == VulkanTool::IndexGetter<ResourceDescription>::Get<T>()) {
               return static_cast<T*>(description.get());
           }else {
               LOG_ERROR("Get Description Type T  is diff with description type !");
               return nullptr;
           }
        }

        ResourceType resourceType{};
    private:
        std::string name;
        uint32_t vulkanResourceIndex = -1;
        std::unordered_set<uint32_t> outputRenderPasses;
        std::unordered_set<uint32_t> inputRenderPasses;
        uint32_t descriptorTypeID = -1; //维护类型安全的

        std::unique_ptr<BaseDescription> description{nullptr};
        //如果没有描述就肯定是外部资源
    };

    class ResourceManager {
    public:
        uint32_t RegisterResource(const std::function<void(std::unique_ptr<ResourceDescription>& )>& Func);
        ResourceDescription* FindResource(const std::string& name);
        ResourceDescription* FindResource(uint32_t index);
    private:
        std::unordered_map<std::string, uint32_t> nameToResourceIndex;
        std::vector<std::unique_ptr<ResourceDescription>> resourceDescriptions;
    };
}

#endif //CAIENGINE_RESOURCEMANAGER_H