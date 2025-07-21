//
// Created by 51092 on 25-7-13.
//

#ifndef DESCRIPTORPOOL_H
#define DESCRIPTORPOOL_H
#include <unordered_map>
#include <queue>
#include "VulkanDevice.h"

namespace FrameWork {
    struct DescriptorPool {
        VkDescriptorPool descriptorPool;
        uint32_t currentSetNum{0};
    };

    class VulkanDescriptorPool {
        //单独一个layout一个pool
        std::unordered_map<VkDescriptorSetLayout, std::vector<DescriptorPool>> descriptorPoolMap;
        //记录对应不使用的descriptorSet
        std::unordered_map<VkDescriptorSetLayout, std::queue<VkDescriptorSet>> unusedDescriptorSetMap; //记住对应的VkDescriptorPool 和 Set
        uint32_t maxSets{100};
        VulkanDevice* vulkanDevice{nullptr};
        VkDescriptorPool createDescriptorPool(VkDescriptorPoolSize poolSize) const;
    public:
        VulkanDescriptorPool();
        ~VulkanDescriptorPool();

        void InitDescriptorPool(FrameWork::VulkanDevice* vulkanDevice);
        void AllocateDescriptorSet(VkDescriptorSetLayout descriptorSetLayout, VkDescriptorType descriptorType, VkDescriptorSet& descriptorSet);
        void RegisterUnusedDescriptorSet(VkDescriptorSetLayout SetLayout, VkDescriptorSet descriptorSet);
        void DestroyDescriptorPool();

    };


}



#endif //DESCRIPTORPOOL_H
