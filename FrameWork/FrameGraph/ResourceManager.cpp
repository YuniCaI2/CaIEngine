//
// Created by cai on 2025/9/12.
//

#include "ResourceManager.h"


uint32_t FG::ResourceManager::RegisterResource(const std::function<void(std::unique_ptr<ResourceDescription>&)> &Func) {
    auto resourceDescription = std::make_unique<ResourceDescription>();
    Func(resourceDescription);
    if (nameToResourceIndex.contains(resourceDescription->GetName())) {
        LOG_WARNING("The name : {} has been existed", resourceDescription->GetName());
        return nameToResourceIndex[resourceDescription->GetName()];
    }
    //保证资源Description的完整性
    if(resourceDescription->isExternal && resourceDescription->GetType() == ResourceType::Texture){
        if(resourceDescription->isPresent){
            resourceDescription->GetDescription<TextureDescription>()->width = vulkanRenderAPI.GetFrameWidth();
            resourceDescription->GetDescription<TextureDescription>()->height = vulkanRenderAPI.GetFrameHeight();
            resourceDescription->GetDescription<TextureDescription>()->format = vulkanRenderAPI.GetVulkanSwapChain().colorFormat;
            resourceDescription->GetDescription<TextureDescription>()->mipLevels = 1;
            resourceDescription->GetDescription<TextureDescription>()->arrayLayers = 1;
            resourceDescription->GetDescription<TextureDescription>()->samples = VK_SAMPLE_COUNT_1_BIT;
            resourceDescription->GetDescription<TextureDescription>()->usages = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }else{
            auto texture = vulkanRenderAPI.getByIndex<FrameWork::Texture>(resourceDescription->vulkanIndex);
            resourceDescription->GetDescription<TextureDescription>()->width = texture->image.extent.width;
            resourceDescription->GetDescription<TextureDescription>()->height = texture->image.extent.height;
            resourceDescription->GetDescription<TextureDescription>()->format = texture->image.format;
            resourceDescription->GetDescription<TextureDescription>()->mipLevels = texture->image.mipLevels;
            resourceDescription->GetDescription<TextureDescription>()->arrayLayers = texture->image.arrayLayers;
            resourceDescription->GetDescription<TextureDescription>()->samples = texture->image.samples;
            resourceDescription->GetDescription<TextureDescription>()->usages = texture->image.usage;
        }
    }
    resourceDescriptions.push_back(std::move(resourceDescription));
    nameToResourceIndex[resourceDescriptions.back()->GetName()] = resourceDescriptions.size() - 1;
    return resourceDescriptions.size() - 1;
}

FG::ResourceDescription * FG::ResourceManager::FindResource(const std::string &name) {
    if (nameToResourceIndex.contains(name)) {
        return resourceDescriptions[nameToResourceIndex[name]].get();
    }else {
        LOG_ERROR("the name: {} is not existed in resourceManager", name);
        return nullptr;
    }
}

FG::ResourceDescription * FG::ResourceManager::FindResource(uint32_t index) {
    if (index >= nameToResourceIndex.size()) {
        LOG_ERROR("the index {} exceed the size of resourceManager", index);
        return nullptr;
    }
    return resourceDescriptions[index].get();
}

void FG::ResourceManager::ClearAliasGroups(){
    aliasGroups.clear();
    resourceDescriptionToAliasGroup.clear();
}

bool FG::ResourceManager::CanAlias(uint32_t resourceDescIndex, uint32_t aliasIndex) {
    auto resourceDesc = resourceDescriptions[resourceDescIndex].get();
    auto group = aliasGroups[aliasIndex];
    uint32_t newFirstTime = resourceDesc->GetFirstUseTime();
    if (resourceDesc->GetType() != group.description->GetResourceType()) {
        return false;
    }
    if (resourceDesc->GetType() == ResourceType::Texture) {
        if (resourceDesc->GetDescription<TextureDescription>()->Equal(group.description)) {
            for (auto& resourceIndex : group.sharedResourceIndices) {
                auto resource = FindResource(resourceIndex);
                auto existingLastTime = resource->GetLastUseTime();
                if (newFirstTime <= existingLastTime) {
                    return false;
                }
            }
            return true;
        }
    }
    if (resourceDesc->GetType() == ResourceType::Buffer) {
        if (resourceDesc->GetDescription<BufferDescription>()->Equal(group.description)) {
            for (auto& resourceIndex : group.sharedResourceIndices) {
                auto resource = FindResource(resourceIndex);
                auto existingLastTime = resource->GetLastUseTime();
                if (newFirstTime >= existingLastTime) {
                    return false;
                }
            }
            return true;
        }
    }
    return false;
}

void FG::ResourceManager::CreateVulkanResource() {
    //先不支持Buffer的创建，因为RenderAPI现无对应的接口
    for (uint32_t index = 0; index < aliasGroups.size(); index++) {
        auto description = aliasGroups[index].description;
        if (description->GetResourceType() == ResourceType::Texture) {
            vulkanRenderAPI.CreateTexture(aliasGroups[index].vulkanIndex , description);
        }else {
            LOG_WARNING("当前VulkanResource 不支持Compute Buffer，等待扩展");
        }
    }
}

void FG::ResourceManager::ResetVulkanResource() {
    for (uint32_t index = 0; index < aliasGroups.size(); index++) {
        auto description = aliasGroups[index].description;
        if (description->GetResourceType() == ResourceType::Texture) {
            if (aliasGroups[index].vulkanIndex != UINT32_MAX) {
                vulkanRenderAPI.DeleteTexture(aliasGroups[index].vulkanIndex);
            }
        }else {
            LOG_WARNING("当前VulkanResource 不支持Compute Buffer，等待扩展");
        }
    }
}

uint32_t FG::ResourceManager::GetVulkanResource(const std::string& name) {
    return GetVulkanResource(nameToResourceIndex[name]);
}

uint32_t FG::ResourceManager::GetVulkanResource(uint32_t resourceIndex) {
    if(resourceDescriptions[resourceIndex]->isExternal){
        return resourceDescriptions[resourceIndex]->vulkanIndex;
    }

    if(!resourceDescriptionToAliasGroup.contains(resourceIndex)){
        LOG_ERROR("The resourceIndex : {} didn't create vulkan resource !", 
            resourceDescriptions[resourceIndex]->GetName());
        return -1;
    }
    return aliasGroups[resourceDescriptionToAliasGroup[resourceIndex]].vulkanIndex;
}

std::vector<FG::AliasGroup> & FG::ResourceManager::GetAliasGroups() {
    return aliasGroups;
}

