//
// Created by 51092 on 25-8-7.
//

#ifndef SLOT_H
#define SLOT_H
#include<unordered_map>
#include "VulkanTool.h"

namespace FrameWork {
    class SlotObjectsContainer {
    public:
        virtual ~SlotObjectsContainer() = default;
        virtual void Update() = 0;
        virtual uint32_t GetDataSize() = 0;
        virtual void* GetObjectPtr() = 0;
    };

    template<typename T, typename... Args>
    class SlotObjectsContainerImpl : public SlotObjectsContainer {
    public:
        SlotObjectsContainerImpl(Args&&... args) : context(std::forward<Args>(args)...) {}
        void Update() override {
            std::apply(
                [this](Args&&... args) {
                    uniformObject.Update(std::forward<Args>(args)...);
                }, context
                );
        }
        uint32_t GetDataSize() override {
            return sizeof(T);
        }
        void* GetObjectPtr() override {
            return &uniformObject;
        }
    private:
        T uniformObject; //直接拥有
        std::tuple<Args...> context;
    };

    class Slot {
    public:
        template<typename T, typename... Args>
        void SetUniformObject(VkShaderStageFlags shaderStage, Args&&... args) {
            auto id = VulkanTool::IndexGetter<Slot>::Get<T>();
            if (auto it = uniformObjects.find(id); it != uniformObjects.end()) {
                return;
            }else {
                auto container = std::make_unique<SlotObjectsContainerImpl<T, Args...>>(std::forward<Args>(args)...);
                uniformObjects.emplace(id ,
                    std::move(container)
                    );
            }

            CreateUniformDescriptorSet(id, shaderStage, sizeof(T));
        }

        void SetStorageBuffer(VkShaderStageFlags shaderStageFlags, uint32_t StorageBufferID);
        void DestroyStorageBuffer(uint32_t StorageBufferID);

        void SetTexture(const std::vector<uint32_t>& textures) {
            textureIDs.insert(textureIDs.end(), textures.begin(), textures.end());
        }
        void SetTexture(VkShaderStageFlags shaderStageFlags,uint32_t textureID);
        void SwitchTexture(VkShaderStageFlags shaderFlags, uint32_t oldTexID, uint32_t newTexID);
        void DestroyTexture(uint32_t textureID);
        std::vector<VkDescriptorSetLayout> GetAllDescriptorSetLayout() const;

        static void DestroyDescriptorSetLayout();
        static VkDescriptorSetLayout CreateTextureDescriptorSetLayout(VkShaderStageFlags shaderStageFlags);
        static VkDescriptorSetLayout CreateStorageDescriptorSetLayout(VkShaderStageFlags shaderStageFlags);
        static VkDescriptorSetLayout CreateUniformDescriptorSet(VkShaderStageFlags shaderStageFlags);

        Slot() = default;
        Slot& operator=(Slot const& other) = delete;
        Slot& operator=(Slot && other) = default;
        Slot(Slot&& other) = default;
        Slot(const Slot& other) = delete;
        ~Slot();


        void Update();//针对Uniform数据
        void Bind(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t firstSet);
        uint32_t GetDescriptorSetsSize() const;


        bool inUse = false;
    private:
        //Uniform的ID隶属于Slot
        std::unordered_map<uint32_t, std::unique_ptr<SlotObjectsContainer>> uniformObjects;
        std::vector<uint32_t> textureIDs;
        std::vector<uint32_t> storageBufferIDs;
        std::unordered_map<uint32_t, uint32_t> textureIDIndexMap;
        std::unordered_map<uint32_t, uint32_t> storageBufferIDIndexMap;
        std::unordered_map<uint32_t, uint32_t> uniformBufferIDIndexMap;

        std::unordered_map<uint32_t, VkDescriptorSetLayout> storageBufferDescriptorSetLayouts;
        std::unordered_map<uint32_t, VkDescriptorSetLayout> textureDescriptorSetLayouts;
        std::unordered_map<uint32_t, VkDescriptorSetLayout> uniformDescriptorSetLayouts;
        std::unordered_map<uint32_t, Buffer> uniformBuffers;

        //保证绑定时维持线性，维持性能
        std::vector<VkDescriptorSet> storageDescriptorSetContainer;
        std::vector<VkDescriptorSet> textureDescriptorSetContainer;
        std::vector<VkDescriptorSet> uniformDescriptorSetContainer;
        std::vector<uint32_t> uniformBufferSizes;//维持线性



        inline static std::unordered_map<VkShaderStageFlags, VkDescriptorSetLayout> globalUniformDescriptorSetLayouts;
        inline static std::unordered_map<VkShaderStageFlags, VkDescriptorSetLayout> globalStorageBufferDescriptorSetLayouts;
        inline static std::unordered_map<VkShaderStageFlags, VkDescriptorSetLayout> globalTextureDescriptorSetLayouts;

        void CreateUniformDescriptorSet(uint32_t uniformObjectID, VkShaderStageFlags shaderStage, VkDeviceSize deviceSize);
        void AddStorageDescriptorSet(uint32_t storageBufferID, VkDescriptorSet descriptorSet);
        void AddTextureDescriptorSet(uint32_t textureID, VkDescriptorSet descriptorSet);
        void DestroyStorageDescriptorSet(uint32_t storageBufferID);
        void DestroyTextureDescriptorSet(uint32_t textureID);

    };


}



#endif //SLOT_H
