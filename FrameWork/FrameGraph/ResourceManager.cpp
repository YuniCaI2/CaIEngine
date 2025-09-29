//
// Created by cai on 2025/9/12.
//

#include "ResourceManager.h"
#include "ThreadPool.h"


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
    reuseResourcePool.clear();
}

bool FG::ResourceManager::CanAlias(uint32_t resourceDescIndex, uint32_t aliasIndex) {
    auto resourceDesc = resourceDescriptions[resourceDescIndex].get();
    auto& group = aliasGroups[aliasIndex];
    uint32_t newFirstTime = resourceDesc->GetFirstUseTime();
    if (resourceDesc->GetType() != group->description->GetResourceType()) {
        return false;
    }
    if (resourceDesc->GetType() == ResourceType::Texture) {
        if (resourceDesc->GetDescription<TextureDescription>()->Equal(group->description)) {
            for (auto& resourceIndex : group->sharedResourceIndices) {
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
        if (resourceDesc->GetDescription<BufferDescription>()->Equal(group->description)) {
            for (auto& resourceIndex : group->sharedResourceIndices) {
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

void FG::ResourceManager::CreateVulkanResource(uint32_t index) {
    auto& group = aliasGroups[index];
    if (group->vulkanIndex != UINT32_MAX) return;

    auto* desc = group->description;
    if (desc->GetResourceType() == ResourceType::Texture) {
            // 从队列中**扫描**第一个到期且兼容的资源
            for (auto iter = reuseResourcePool.begin(); iter != reuseResourcePool.end(); ++iter) {
                auto& [resource, i] = *iter;
                if (i == 0) {
                    if (desc->Equal(resource.baseDescription)) {
                        group->vulkanIndex = resource.resourceIndex;
                        group->resolveVulkanIndex = resource.resolveIndex;
                        group->isReset = false;
                        reuseResourcePool.erase(iter);
                        return;
                    }
                }
            }
    } else {
        LOG_WARNING("当前VulkanResource 不支持Compute Buffer，等待扩展");
    }

    // 没找到可复用 => 新建
    uint32_t vulkanIndex = UINT32_MAX, resolveVulkanIndex = UINT32_MAX;
    vulkanRenderAPI.CreateTexture(vulkanIndex, resolveVulkanIndex, desc);
    group->vulkanIndex = vulkanIndex;
    group->resolveVulkanIndex = resolveVulkanIndex;
    group->isReset = false;
}

void FG::ResourceManager::CreateVulkanResources(ThreadPool& threadPool) {

    for (int i = 0; i < aliasGroups.size(); ++i) {
        CreateVulkanResource(i);
    }
}

void FG::ResourceManager::ResetVulkanResources() {
    for (uint32_t index = 0; index < aliasGroups.size(); index++) {
        auto description = aliasGroups[index]->description;
        if (description->GetResourceType() == ResourceType::Texture) {
            ResetVulkanResource(index);
        }else {
            LOG_WARNING("当前VulkanResource 不支持Compute Buffer，等待扩展");
        }
    }
}

void FG::ResourceManager::ResetVulkanResource(uint32_t aliasIndex) {
    auto& group = aliasGroups[aliasIndex];
    if (group->vulkanIndex != UINT32_MAX) {
        reuseResourcePool.emplace(
            ReuseResource{ group->vulkanIndex, group->resolveVulkanIndex, group->description}, MAX_FRAME);
        group->vulkanIndex = UINT32_MAX;
        group->resolveVulkanIndex = UINT32_MAX;
    }
    group->isReset = true;
}

void FG::ResourceManager::UpdateReusePool() {
    for (auto& [resource, frame] : reuseResourcePool) {
        if (frame > 0)
            frame--;
    }

    //超时销毁先不添加，因为现在是静态的
}


uint32_t FG::ResourceManager::GetVulkanIndex(const std::string& name) {
    return GetVulkanIndex(nameToResourceIndex[name]);
}

uint32_t FG::ResourceManager::GetVulkanIndex(uint32_t resourceIndex) {
    if (resourceDescriptions[resourceIndex]->isExternal) return resourceDescriptions[resourceIndex]->vulkanIndex;
    auto ag = resourceDescriptionToAliasGroup[resourceIndex];
    auto& a = aliasGroups[ag];

    return a->vulkanIndex;

}

uint32_t FG::ResourceManager::GetVulkanResolveIndex(uint32_t resourceIndex) {
    if (resourceDescriptions[resourceIndex]->isExternal) {
        LOG_ERROR("The external resourceIndex : {} , Can't get resolve index", resourceDescriptions[resourceIndex]->GetName());
        return -1;
    }
    if (resourceDescriptions[resourceIndex]->GetType() == ResourceType::Texture) {
        auto ag = resourceDescriptionToAliasGroup[resourceIndex];
        auto& a = aliasGroups[ag];

        return a->resolveVulkanIndex;
    }else {
        LOG_ERROR("The resource type is Buffer ,didnt have resolve index");
        return -1;
    }
}

std::vector<std::unique_ptr<FG::AliasGroup>> & FG::ResourceManager::GetAliasGroups() {
    return aliasGroups;
}

