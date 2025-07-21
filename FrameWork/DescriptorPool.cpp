//
// Created by 51092 on 25-7-13.
//

#include "DescriptorPool.h"
#include <ranges>
#include "VulkanTool.h"

//descriptor Pool策略
/*
descriptor Pool根据不同的layout进行分配，来实现不使用的Set进行重用
 */
VkDescriptorPool FrameWork::VulkanDescriptorPool::createDescriptorPool(VkDescriptorPoolSize poolSize) const {
    VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .maxSets = maxSets,
        .poolSizeCount = 1,
        .pPoolSizes = &poolSize,
    };
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VK_CHECK_RESULT(vkCreateDescriptorPool(vulkanDevice->logicalDevice, &poolInfo, nullptr, &descriptorPool));
    return descriptorPool;
}

FrameWork::VulkanDescriptorPool::VulkanDescriptorPool() {
}

FrameWork::VulkanDescriptorPool::~VulkanDescriptorPool() {
}

void FrameWork::VulkanDescriptorPool::InitDescriptorPool(FrameWork::VulkanDevice *vulkanDevice) {
    this->vulkanDevice = vulkanDevice;
}

void FrameWork::VulkanDescriptorPool::AllocateDescriptorSet(VkDescriptorSetLayout descriptorSetLayout,
    VkDescriptorType descriptorType, VkDescriptorSet&descriptorSet) {
    //实现查找hashmap
    if (unusedDescriptorSetMap.contains(descriptorSetLayout)) {
        descriptorSet = unusedDescriptorSetMap[descriptorSetLayout].front();
        unusedDescriptorSetMap[descriptorSetLayout].pop();
    }
    if (! descriptorPoolMap.contains(descriptorSetLayout)) {
        //不存在则需要创建
        VkDescriptorPoolSize poolSize = {
            .type = descriptorType,
            .descriptorCount = maxSets
        };
        //此处一个Set对应一个bind，方便管理，也进一步减少descriptorSetLayout的种类使其通用
        descriptorPoolMap[descriptorSetLayout].emplace_back(createDescriptorPool(poolSize), 1);
        auto& descriptorPool = descriptorPoolMap[descriptorSetLayout].back();
        VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext = 0,
            .descriptorPool = descriptorPool.descriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &descriptorSetLayout,
        };
        VK_CHECK_RESULT(vkAllocateDescriptorSets(vulkanDevice->logicalDevice, &descriptorSetAllocateInfo, &descriptorSet));
    }else {
        if (descriptorPoolMap[descriptorSetLayout].back().currentSetNum < maxSets) {
            auto& descriptorPool = descriptorPoolMap[descriptorSetLayout].back();
            descriptorPool.currentSetNum++;
            VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .pNext = 0,
                .descriptorPool = descriptorPool.descriptorPool,
                .descriptorSetCount = 1,
                .pSetLayouts = &descriptorSetLayout,
            };
            VK_CHECK_RESULT(vkAllocateDescriptorSets(vulkanDevice->logicalDevice, &descriptorSetAllocateInfo, &descriptorSet));
        }else {
            VkDescriptorPoolSize poolSize = {
                .type = descriptorType,
                .descriptorCount = maxSets
            };
            //此处一个Set对应一个bind，方便管理，也进一步减少descriptorSetLayout的种类使其通用
            descriptorPoolMap[descriptorSetLayout].emplace_back(createDescriptorPool(poolSize), 1);
            auto& descriptorPool = descriptorPoolMap[descriptorSetLayout].back();
            VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .pNext = 0,
                .descriptorPool = descriptorPool.descriptorPool,
                .descriptorSetCount = 1,
                .pSetLayouts = &descriptorSetLayout,
            };
            VK_CHECK_RESULT(vkAllocateDescriptorSets(vulkanDevice->logicalDevice, &descriptorSetAllocateInfo, &descriptorSet));
        }
    }
}

void FrameWork::VulkanDescriptorPool::RegisterUnusedDescriptorSet(VkDescriptorSetLayout SetLayout,
    VkDescriptorSet descriptorSet) {
    unusedDescriptorSetMap[SetLayout].emplace(descriptorSet);
}


void FrameWork::VulkanDescriptorPool::DestroyDescriptorPool() {
    for (auto& [_ , descriptorPools] : descriptorPoolMap) {
        for (auto& d : descriptorPools) {
            vkDestroyDescriptorPool(vulkanDevice->logicalDevice, d.descriptorPool, nullptr);
        }
    }
}

