//
// Created by 51092 on 25-7-13.
//

#include "DescriptorPool.h"
#include "vulkanFrameWork.h"
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
    pendingDescriptorSets.resize(vulkanRenderAPI.MaxFrame);
}

void FrameWork::VulkanDescriptorPool::AllocateDescriptorSet(VkDescriptorSetLayout descriptorSetLayout,
    VkDescriptorType descriptorType, VkDescriptorSet&descriptorSet) {
    std::lock_guard<std::mutex> lock(mutex);
    //实现查找hashmap
    if (unusedDescriptorSetMap.contains(descriptorSetLayout)) {
        descriptorSet = unusedDescriptorSetMap[descriptorSetLayout].front();
        unusedDescriptorSetMap[descriptorSetLayout].pop();
        return;
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
    if (descriptorSet == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to allocate descriptor set, the descriptor set is null");
    }
}

void FrameWork::VulkanDescriptorPool::RegisterUnusedDescriptorSet(VkDescriptorSetLayout SetLayout,
    VkDescriptorSet descriptorSet) {
    std::lock_guard<std::mutex> lock(mutex);
    pendingDescriptorSets[vulkanRenderAPI.GetCurrentFrame()][SetLayout].push(descriptorSet);
}

void FrameWork::VulkanDescriptorPool::ClearPendingQueue() {
    std::lock_guard<std::mutex> lock(mutex);
    auto frame = vulkanRenderAPI.GetCurrentFrame() + 1;
    frame = frame % vulkanRenderAPI.MaxFrame;
    if (! pendingDescriptorSets[frame].empty()) {
        for (auto& p : pendingDescriptorSets[frame]) {
            while (p.second.size() > 0) {
                unusedDescriptorSetMap[p.first].push(p.second.front());
                p.second.pop();
            }
        }
        //一起清理
        pendingDescriptorSets[frame].clear();
    }
}


void FrameWork::VulkanDescriptorPool::DestroyDescriptorPool() {
    for (auto& [_ , descriptorPools] : descriptorPoolMap) {
        for (auto& d : descriptorPools) {
            vkDestroyDescriptorPool(vulkanDevice->logicalDevice, d.descriptorPool, nullptr);
        }
    }
}

