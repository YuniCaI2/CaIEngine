//
// Created by 51092 on 25-8-7.
//

#include "Slot.h"
#include "vulkanFrameWork.h"


void FrameWork::Slot::SetStorageBuffer(VkShaderStageFlags shaderStageFlags, uint32_t storageBufferID) {
    if (std::find(storageBufferIDs.begin(), storageBufferIDs.end(), storageBufferID)
        != storageBufferIDs.end()) {
        return;
    }
    auto storageBuffer = vulkanRenderAPI.getByIndex<FrameWork::StorageBuffer>(storageBufferID);
    if (auto it = globalStorageBufferDescriptorSetLayouts.find(shaderStageFlags);
        it != globalStorageBufferDescriptorSetLayouts.end()) {
        storageBufferDescriptorSetLayouts[storageBufferID] = it->second;
    } else {
        auto storageBufferDescriptorSetLayout =
                vulkanRenderAPI.CreateDescriptorSetLayout(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, shaderStageFlags);
        storageBufferDescriptorSetLayouts[storageBufferID] = storageBufferDescriptorSetLayout;
        globalStorageBufferDescriptorSetLayouts[shaderStageFlags] = storageBufferDescriptorSetLayout;
    }
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    try {
        vulkanRenderAPI.vulkanDescriptorPool.AllocateDescriptorSet(
            storageBufferDescriptorSetLayouts[storageBufferID], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, descriptorSet);
    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
        exit(1);
    }
    VkDescriptorBufferInfo descriptorBufferInfo = {
        .buffer = storageBuffer->buffer.buffer,
        .offset = 0,
        .range = VK_WHOLE_SIZE
    };

    VkWriteDescriptorSet descriptorWrite = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = descriptorSet,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pImageInfo = nullptr,
        .pBufferInfo = &descriptorBufferInfo,
        .pTexelBufferView = nullptr
    };
    vkUpdateDescriptorSets(vulkanRenderAPI.device, 1, &descriptorWrite, 0, nullptr);
    storageBufferIDs.push_back(storageBufferID);
    AddStorageDescriptorSet(storageBufferID, descriptorSet);
}

void FrameWork::Slot::DestroyStorageBuffer(uint32_t storageBufferID) {
    if (auto it =
                std::find(storageBufferIDs.begin(), storageBufferIDs.end(), storageBufferID);
        it != storageBufferIDs.end()) {
        std::swap(*it, storageBufferIDs.back());
        storageBufferIDs.pop_back();
        auto descriptorSet = storageDescriptorSetContainer[storageBufferIDIndexMap[storageBufferID]];
        DestroyStorageDescriptorSet(storageBufferID);
        vulkanRenderAPI.vulkanDescriptorPool.
                RegisterUnusedDescriptorSet(storageBufferDescriptorSetLayouts[storageBufferID], descriptorSet);
        storageBufferDescriptorSetLayouts.erase(storageBufferID);
    }
}

void FrameWork::Slot::SetTexture(VkShaderStageFlags shaderStageFlags, uint32_t textureID) {
    if (std::find(textureIDs.begin(), textureIDs.end(), textureID)
        != textureIDs.end()) {
        return;
    }
    if (auto it = globalTextureDescriptorSetLayouts.find(shaderStageFlags);
        it != globalTextureDescriptorSetLayouts.end()) {
        textureDescriptorSetLayouts[textureID] = it->second;
    } else {
        auto textureDescriptorSetLayout =
                vulkanRenderAPI.CreateDescriptorSetLayout(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, shaderStageFlags);
        textureDescriptorSetLayouts[textureID] = textureDescriptorSetLayout;
        globalTextureDescriptorSetLayouts[shaderStageFlags] = textureDescriptorSetLayout;
    }
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    try {
        vulkanRenderAPI.vulkanDescriptorPool.AllocateDescriptorSet(
            textureDescriptorSetLayouts[textureID], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            descriptorSet
        );
    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
        exit(1);
    }
    auto texture = vulkanRenderAPI.getByIndex<FrameWork::Texture>(textureID);
    VkDescriptorImageInfo descriptorImageInfo = {
        .sampler = texture->sampler,
        .imageView = texture->imageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };
    VkWriteDescriptorSet descriptorWrite = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = descriptorSet,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &descriptorImageInfo,
        .pBufferInfo = nullptr,
        .pTexelBufferView = nullptr,
    };
    vkUpdateDescriptorSets(vulkanRenderAPI.device, 1, &descriptorWrite, 0, nullptr);
    AddTextureDescriptorSet(textureID, descriptorSet);
    textureIDs.push_back(textureID);
}

void FrameWork::Slot::SwitchTexture(VkShaderStageFlags shaderStageFlags, uint32_t oldTexID, uint32_t newTexID) {
    //先添加在末尾
    SetTexture(shaderStageFlags,newTexID);
    //再删除，此时末尾的位置会代替删除的位置方便popback
    DestroyTextureDescriptorSet(oldTexID);

}



VkDescriptorSetLayout FrameWork::Slot::CreateTextureDescriptorSetLayout(VkShaderStageFlags shaderStageFlags) {
    if (globalTextureDescriptorSetLayouts.find(shaderStageFlags) == globalTextureDescriptorSetLayouts.end()) {
        auto descriptorSetLayout = vulkanRenderAPI.CreateDescriptorSetLayout(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                            shaderStageFlags);
        globalTextureDescriptorSetLayouts[shaderStageFlags] = descriptorSetLayout;
    }
    return globalTextureDescriptorSetLayouts[shaderStageFlags];
}

VkDescriptorSetLayout FrameWork::Slot::CreateStorageDescriptorSetLayout(VkShaderStageFlags shaderStageFlags) {
    if (globalStorageBufferDescriptorSetLayouts.find(shaderStageFlags) == globalStorageBufferDescriptorSetLayouts.end()) {
        auto descriptorSetLayout = vulkanRenderAPI.CreateDescriptorSetLayout(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, shaderStageFlags);
        globalStorageBufferDescriptorSetLayouts[shaderStageFlags] = descriptorSetLayout;
    }
    return globalStorageBufferDescriptorSetLayouts[shaderStageFlags];
}

VkDescriptorSetLayout FrameWork::Slot::CreateUniformDescriptorSetLayout(VkShaderStageFlags shaderStageFlags) {
    if (globalUniformDescriptorSetLayouts.find(shaderStageFlags) == globalUniformDescriptorSetLayouts.end()) {
        auto descriptorSetLayout = vulkanRenderAPI.CreateDescriptorSetLayout(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
                                                                    shaderStageFlags);
        globalUniformDescriptorSetLayouts[shaderStageFlags] = descriptorSetLayout;
    }
    return globalUniformDescriptorSetLayouts[shaderStageFlags];
}

uint32_t FrameWork::Slot::GetDescriptorSetsSize() const {
    return uniformDescriptorSetContainer.size() +
        storageDescriptorSetContainer.size() + textureDescriptorSetContainer.size();
}

void FrameWork::Slot::DestroyDescriptorSetLayout() {
    for (auto &[_, setLayout]: globalTextureDescriptorSetLayouts) {
        vkDestroyDescriptorSetLayout(vulkanRenderAPI.GetVulkanDevice()->logicalDevice, setLayout, nullptr);
    }
    for (auto &[_, setLayout]: globalUniformDescriptorSetLayouts) {
        vkDestroyDescriptorSetLayout(vulkanRenderAPI.GetVulkanDevice()->logicalDevice, setLayout, nullptr);
    }
    for (auto &[_, setLayout]: globalStorageBufferDescriptorSetLayouts) {
        vkDestroyDescriptorSetLayout(vulkanRenderAPI.GetVulkanDevice()->logicalDevice, setLayout, nullptr);
    }
}

void FrameWork::Slot::DestroyTexture(uint32_t textureID) {
    if (auto it = std::find(textureIDs.begin(),
                            textureIDs.end(), textureID); it != textureIDs.end()) {
        std::swap(*it, textureIDs.back());
        textureIDs.pop_back();
        auto descriptorSet = textureDescriptorSetContainer[textureIDIndexMap[textureID]];
        DestroyTextureDescriptorSet(textureID);
        vulkanRenderAPI.vulkanDescriptorPool.
                RegisterUnusedDescriptorSet(textureDescriptorSetLayouts[textureID], descriptorSet);
        textureDescriptorSetLayouts.erase(textureID);
    }
}

std::vector<VkDescriptorSetLayout> FrameWork::Slot::GetAllDescriptorSetLayout() const {
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts(storageBufferDescriptorSetLayouts.size()
                                                            + textureDescriptorSetLayouts.size() +
                                                            uniformDescriptorSetLayouts.size());
    int i = 0;
    for (auto &[_, setLayout]: uniformDescriptorSetLayouts) {
        descriptorSetLayouts[i++] = setLayout;
    }
    for (auto &[_, setLayout]: textureDescriptorSetLayouts) {
        descriptorSetLayouts[i++] = setLayout;
    }
    for (auto &[_, setLayout]: storageBufferDescriptorSetLayouts) {
        descriptorSetLayouts[i++] = setLayout;
    }
    return descriptorSetLayouts;
}

FrameWork::Slot::~Slot() {
    for (auto [_, buffer]: uniformBuffers) {
        buffer.destroy();
    }
}

void FrameWork::Slot::Update() {
    for (auto &[id, container]: uniformObjects) {
        container->Update();
        auto &buffer = uniformBuffers[id];

        memcpy(
            static_cast<char *>(buffer.mapped) + vulkanRenderAPI.currentFrame * uniformBufferSizes[uniformBufferIDIndexMap[
                id]] / MAX_FRAME,
            container->GetObjectPtr(), container->GetDataSize());
    }
}

void FrameWork::Slot::Bind(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t firstSet) {
    if (storageDescriptorSetContainer.size() != 0) {
        for (int i = 0; i < uniformDescriptorSetContainer.size(); i++) {
            uint32_t descriptorOffset =
                    vulkanRenderAPI.currentFrame * uniformBufferSizes[i]
                    / vulkanRenderAPI.MaxFrame;
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, i + firstSet, 1,
                                    &uniformDescriptorSetContainer[i], 1, &descriptorOffset);
        }
        if (storageDescriptorSetContainer.size() != 0)
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                pipelineLayout, uniformDescriptorSetContainer.size() + firstSet, storageDescriptorSetContainer.size(),
                                storageDescriptorSetContainer.data(), 0, nullptr);
    } else {
        for (int i = 0; i < uniformDescriptorSetContainer.size(); i++) {
            uint32_t descriptorOffset =
                    vulkanRenderAPI.currentFrame * uniformBufferSizes[i]
                    / vulkanRenderAPI.MaxFrame;
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, i + firstSet, 1,
                                    &uniformDescriptorSetContainer[i], 1, &descriptorOffset);
        }
        if (textureDescriptorSetLayouts.size() != 0)
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipelineLayout, uniformDescriptorSetContainer.size() + firstSet, textureDescriptorSetContainer.size(),
                                textureDescriptorSetContainer.data(), 0, nullptr);
    }
}

void FrameWork::Slot::CreateUniformDescriptorSet(uint32_t uniformObjectID, VkShaderStageFlags shaderStage,
                                                 VkDeviceSize deviceSize) {
    VkDescriptorSetLayout descriptorSetLayout{};
    VkDescriptorSet descriptorSet{};
    if (auto it = globalUniformDescriptorSetLayouts.find(shaderStage);
        it != globalUniformDescriptorSetLayouts.end()) {
        descriptorSetLayout = it->second;
    } else {
        descriptorSetLayout = vulkanRenderAPI.CreateDescriptorSetLayout(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
                                                                        shaderStage);
        globalUniformDescriptorSetLayouts[shaderStage] = descriptorSetLayout;
    }
    uniformDescriptorSetLayouts[uniformObjectID] = descriptorSetLayout;
    Buffer uniformBuffer{};
    auto alignment = vulkanRenderAPI.vulkanDevice->properties.limits.minUniformBufferOffsetAlignment;
    uint32_t dynamicSize = (deviceSize + alignment - 1) & ~(alignment - 1); //实现对齐
    vulkanRenderAPI.vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                                               , &uniformBuffer, dynamicSize * MAX_FRAME
    );
    uniformBuffer.map();
    //一个字节一个字节
    for (int k = 0; k < MAX_FRAME; k++) {
        memcpy(static_cast<char *>(uniformBuffer.mapped) +
               dynamicSize * k, uniformObjects[uniformObjectID]->GetObjectPtr(), dynamicSize);
    }
    try {
        vulkanRenderAPI.vulkanDescriptorPool.AllocateDescriptorSet(descriptorSetLayout,
                                                                   VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
                                                                   descriptorSet);
    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
        exit(1);
    }
    VkDescriptorBufferInfo uniformBufferInfo = {
        .buffer = uniformBuffer.buffer,
        .offset = 0,
        .range = deviceSize
        //这里是单次的访问大小而不是整个buffer大小
    };

    VkWriteDescriptorSet descriptorWrite = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = descriptorSet,
        .dstBinding = 0,
        .dstArrayElement = 0, //描述符元素数组开始的下标
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
        .pImageInfo = nullptr,
        .pBufferInfo = &uniformBufferInfo,
        .pTexelBufferView = nullptr,
    };
    vkUpdateDescriptorSets(vulkanRenderAPI.device, 1, &descriptorWrite, 0, nullptr);
    uniformBuffers[uniformObjectID] = uniformBuffer;
    uint32_t len = uniformDescriptorSetContainer.size();
    uniformBufferIDIndexMap[uniformObjectID] = len;
    uniformDescriptorSetContainer.push_back(descriptorSet);
    uniformBufferSizes.push_back(dynamicSize * MAX_FRAME);
}

void FrameWork::Slot::AddStorageDescriptorSet(uint32_t storageBufferID, VkDescriptorSet descriptorSet) {
    auto len = storageDescriptorSetContainer.size();
    storageDescriptorSetContainer.push_back(descriptorSet);
    storageBufferIDIndexMap[storageBufferID] = len;
}

void FrameWork::Slot::AddTextureDescriptorSet(uint32_t textureID, VkDescriptorSet descriptorSet) {
    auto len = textureDescriptorSetContainer.size();
    textureDescriptorSetContainer.push_back(descriptorSet);
    textureIDIndexMap[textureID] = len;
}

void FrameWork::Slot::DestroyStorageDescriptorSet(uint32_t storageBufferID) {
    auto index = storageBufferIDIndexMap[storageBufferID];
    uint32_t lastId = {};
    for (auto &[id, conId]: storageBufferIDIndexMap) {
        if (conId == storageDescriptorSetContainer.size() - 1) {
            lastId = id;
        }
    }
    std::swap(storageDescriptorSetContainer[index], storageDescriptorSetContainer.back());
    storageDescriptorSetContainer.pop_back();
    storageBufferIDIndexMap.erase(storageBufferID);
    storageBufferIDIndexMap[lastId] = index;
    if (storageBufferID != lastId) {
        //防止只有一个元素的时候将其送回
        storageBufferIDIndexMap[storageBufferID] = index;
    }
}

void FrameWork::Slot::DestroyTextureDescriptorSet(uint32_t textureID) {
    auto index = textureIDIndexMap[textureID];
    uint32_t lastId = {};
    for (auto &[id, conId]: textureIDIndexMap) {
        if (conId == textureDescriptorSetContainer.size() - 1) {
            lastId = id;
        }
    }
    std::swap(textureDescriptorSetContainer[index], textureDescriptorSetContainer.back());
    textureDescriptorSetContainer.pop_back();
    textureIDIndexMap.erase(textureID);
    if (textureID != lastId) {
        textureIDIndexMap[lastId] = index;
    }
}
