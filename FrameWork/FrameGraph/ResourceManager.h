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
    class FrameGraph;
}

namespace FG {
    enum class ResourceType {
        Texture,
        Buffer
    };

    // 基础描述接口
    struct BaseDescription {
        virtual ~BaseDescription() = default;
        virtual bool Equal(BaseDescription* description) = 0;
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

        bool Equal( BaseDescription *description) override {
            if (description->GetResourceType() != GetResourceType()) return false;
            auto texDesc = static_cast<TextureDescription*>(description);
            return texDesc->width == width && texDesc->height == height && texDesc->mipLevels == mipLevels && texDesc->format == format
            && texDesc->samples == samples && texDesc->usages == usages && texDesc->arrayLayers == arrayLayers;
        }

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
        bool Equal(BaseDescription *description) override {
            if (description->GetResourceType() != GetResourceType()) return false;
            auto bufferDesc = static_cast<BufferDescription*>(description);
            return bufferDesc->usages == usages && bufferDesc->size == size;
        }

        ResourceType GetResourceType() const override {
            return ResourceType::Buffer;
        }
    };

    //别名组资源，其直接对应Vulkan资源
    struct AliasGroup {
        std::vector<uint32_t> sharedResourceIndices;
        uint32_t vulkanIndex{UINT32_MAX};
        BaseDescription* description{}; //资源描述
    };

    class ResourceDescription {
    public:
        ResourceDescription() = default;

        virtual ~ResourceDescription() = default;

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
        ResourceDescription& SetFirstUseTime(uint32_t time) {
            firstUseTime = time;
            return *this;
        }
        ResourceDescription& SetLastUseTime(uint32_t time) {
            lastUseTime = time;
            return *this;
        }
        ResourceDescription& ResetUseTime() {
            firstUseTime = UINT32_MAX;
            lastUseTime = 0;
            return *this;
        }

        uint32_t GetFirstUseTime() {return firstUseTime;}
        uint32_t GetLastUseTime() {return lastUseTime;}
        ResourceType GetType() const { return resourceType; }
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
        bool isExternal = false; //是否是外部导入的资源
        bool isPresent = false; //是否是交换链的资源---交换链的资源不参与别名化
        uint32_t vulkanIndex = -1; //对应的Vulkan资源Index
    private:
        std::string name;
        std::unordered_set<uint32_t> outputRenderPasses;
        std::unordered_set<uint32_t> inputRenderPasses;
        uint32_t descriptorTypeID = -1; //维护类型安全的
        std::unique_ptr<BaseDescription> description{nullptr};


        uint32_t firstUseTime{UINT32_MAX};
        uint32_t lastUseTime{0};

    };
    using RegisterFunc = std::function<void(std::unique_ptr<ResourceDescription>&)>;

    class ResourceManager {
    public:
        uint32_t RegisterResource(const std::function<void(std::unique_ptr<ResourceDescription>& )>& Func);
        ResourceDescription* FindResource(const std::string& name);
        ResourceDescription* FindResource(uint32_t index);
        uint32_t GetVulkanResource(const std::string& name);
        uint32_t GetVulkanResource(uint32_t resourceIndex);
        void ClearAliasGroups();
        std::vector<AliasGroup>& GetAliasGroups();
    private:
        friend FrameGraph;
        bool CanAlias(uint32_t resourceIndex, uint32_t aliasIndex);
        //aliasGroup Index创建
        void CreateVulkanResource();//生成Alias Group
        void ResetVulkanResource();


        std::unordered_map<std::string, uint32_t> nameToResourceIndex;
        std::vector<std::unique_ptr<ResourceDescription>> resourceDescriptions;
        std::vector<AliasGroup> aliasGroups;
        std::unordered_map<uint32_t, uint32_t> resourceDescriptionToAliasGroup;
    };
}

#endif //CAIENGINE_RESOURCEMANAGER_H