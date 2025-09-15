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

uint32_t FG::ResourceManager::GetVulkanResourceID(const std::string &name) {
    return aliasGroups[nameToResourceIndex[name]].vulkanIndex;
}

uint32_t FG::ResourceManager::GetVulkanResourceID(uint32_t index) {
    return aliasGroups[index].vulkanIndex;
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

uint32_t FG::ResourceManager::CreateVulkanResource(uint32_t index) {
    //先不支持Buffer的创建，因为RenderAPI现无对应的接口
    if (resourceDescriptions[index]->GetType() == ResourceType::Proxy) {
        LOG_WARNING("Proxy resource : {} can't create", resourceDescriptions[index]->GetName());
        //返回错误值
        return -1;
    }else if (resourceDescriptions[index]->GetType() == ResourceType::Texture) {
        uint32_t rt = 0;
        vulkanRenderAPI.CreateTexture(index,
            resourceDescriptions[index]->GetDescription<TextureDescription>());
        return rt;
    }else {
        LOG_WARNING("当前VulkanResource 不支持Compute Buffer，等待扩展");
        return -1;
    }
}

std::vector<FG::AliasGroup> & FG::ResourceManager::GetAliasGroups() {
    return aliasGroups;
}

