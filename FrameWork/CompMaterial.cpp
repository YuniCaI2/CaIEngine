//
// Created by cai on 2025/9/23.
//

#include "CompMaterial.h"
#include "vulkanFrameWork.h"

FrameWork::CompMaterial * FrameWork::CompMaterial::Create(uint32_t& materialID, uint32_t &shaderID) {
    for (int i = 0; i < compMaterialPool.size(); i++) {
        if (compMaterialPool[i] == nullptr) {
            materialID = i;
            compMaterialPool[i] = new FrameWork::CompMaterial(shaderID);
            return compMaterialPool[i];
        }
    }
    materialID = compMaterialPool.size();
    compMaterialPool.push_back(new FrameWork::CompMaterial(shaderID));
    return compMaterialPool.back();
}

void FrameWork::CompMaterial::Destroy(const uint32_t &id) {
    if (id >= compMaterialPool.size()) {
        LOG_ERROR("Comp Material ID: {}  out of range", id);
        return;
    }
    delete compMaterialPool[id];
    compMaterialPool[id] = nullptr;
}

FrameWork::CompMaterial * FrameWork::CompMaterial::Get(uint32_t &id) {
    if (id >= compMaterialPool.size() || compMaterialPool[id] == nullptr) {
        LOG_ERROR("Comp Material ID: {}  is not existed", id);
    }
    return compMaterialPool[id];
}

void FrameWork::CompMaterial::DestroyAll() {
    for (int i = 0; i < compMaterialPool.size(); i++) {
        Destroy(i);
    }
}

bool FrameWork::CompMaterial::exist(uint32_t id) {
    if (id >= compMaterialPool.size() || compMaterialPool[id] == nullptr) {
        LOG_ERROR("Comp Material ID: {}  is not existed", id);
        return false;
    }
    return true;
}

FrameWork::CompMaterial::~CompMaterial() {
    if (compDataID != UINT32_MAX) {
        vulkanRenderAPI.DeleteCompMaterialData(compDataID);
    }
}

FrameWork::CompMaterial & FrameWork::CompMaterial::SetTexture(const std::string &name, uint32_t id) {
     if (!CompShader::exist(shaderRef)) {
        LOG_ERROR("Failed to set texture for material \"{}\" , Because shader is invalidation", name);
        return *this;
     }
    auto shaderInfo = CompShader::Get(shaderRef)->GetShaderInfo();
    auto materialData = vulkanRenderAPI.getByIndex<FrameWork::CompMaterialData>(compDataID);
    uint32_t binding = -1;
    for (int i = 0; i < shaderInfo.shaderProperties.textureProperties.size(); i++) {
        if (name == shaderInfo.shaderProperties.textureProperties[i].name) {
            binding = shaderInfo.shaderProperties.textureProperties[i].binding;
            break;
        }
    }
    if (binding == -1) {
        LOG_ERROR("Can't find name : {} in shader: {}", name, shaderRef);
        return *this;
    }

    auto texture = vulkanRenderAPI.getByIndex<FrameWork::Texture>(id);
    if (texture == nullptr) {
        LOG_ERROR("Failed to set texture for material \"{}\", the texture is nullptr ", name);
        return *this;
    }
    if (texture->inUse == false) {
        LOG_ERROR("Failed set texture name: \" {} \", the texture inUse == false", name);
        return *this;
    }
    for (auto& set : materialData->descriptorSets) {
        VkDescriptorImageInfo descriptorInfo = {
            .sampler = texture->sampler,
            .imageView = texture->imageView,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL
        };
        VkWriteDescriptorSet descriptorWrite = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = set,
            .dstBinding = binding,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &descriptorInfo,
        };
        vkUpdateDescriptorSets(vulkanRenderAPI.GetVulkanDevice()->logicalDevice,
                               1, &descriptorWrite, 0, nullptr);
    }

    return *this;
}

FrameWork::CompMaterial & FrameWork::CompMaterial::SetAttachment(const std::string &name, uint32_t id) {
     if (!CompShader::exist(shaderRef)) {
        LOG_ERROR("Failed to set texture for material \"{}\" , Because shader is invalidation", name);
        return *this;
     }
    auto shaderInfo = CompShader::Get(shaderRef)->GetShaderInfo();
    auto materialData = vulkanRenderAPI.getByIndex<FrameWork::CompMaterialData>(compDataID);
    uint32_t binding = -1;
    for (int i = 0; i < shaderInfo.shaderProperties.textureProperties.size(); i++) {
        if (name == shaderInfo.shaderProperties.textureProperties[i].name) {
            binding = shaderInfo.shaderProperties.textureProperties[i].binding;
            break;
        }
    }
    if (binding == -1) {
        LOG_ERROR("Can't find name : {} in shader: {}", name, shaderRef);
        return *this;
    }

    auto texture = vulkanRenderAPI.getByIndex<FrameWork::Texture>(id);
    if (texture == nullptr) {
        LOG_ERROR("Failed to set texture for material \"{}\", the texture is nullptr ", name);
        return *this;
    }
    if (texture->inUse == false) {
        LOG_ERROR("Failed set texture name: \" {} \", the texture inUse == false", name);
        return *this;
    }
    VkDescriptorImageInfo descriptorInfo = {
        .sampler = texture->sampler,
        .imageView = texture->imageView,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL
    };
    VkWriteDescriptorSet descriptorWrite = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = materialData->descriptorSets[vulkanRenderAPI.GetCurrentFrame()],
        .dstBinding = binding,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &descriptorInfo,
    };
    vkUpdateDescriptorSets(vulkanRenderAPI.GetVulkanDevice()->logicalDevice,
                           1, &descriptorWrite, 0, nullptr);

    return *this;
}

FrameWork::CompMaterial & FrameWork::CompMaterial::
SetStorageImage2D(const std::string &name, uint32_t id, uint32_t baseMipmap, bool isStatic) {
     if (!CompShader::exist(shaderRef)) {
        LOG_ERROR("Failed to set texture for material \"{}\" , Because shader is invalidation", name);
        return *this;
     }
    auto shaderInfo = CompShader::Get(shaderRef)->GetShaderInfo();
    auto materialData = vulkanRenderAPI.getByIndex<FrameWork::CompMaterialData>(compDataID);
    uint32_t binding = -1;
    for (int i = 0; i < shaderInfo.ssbos.size(); i++) {
        if (shaderInfo.ssbos[i].name == name) {
            if (shaderInfo.ssbos[i].type != StorageObjectType::Buffer) {
                binding = shaderInfo.ssbos[i].binding;
            }
        }
    }
    if (binding == -1) {
        LOG_ERROR("Can't find storageImage name : {} in shader: {}", name, shaderRef);
        return *this;
    }

    auto texture = vulkanRenderAPI.getByIndex<FrameWork::Texture>(id);

    if (texture == nullptr) {
        LOG_ERROR("Failed to set storageImage for material \"{}\", the storageImage is nullptr ", name);
        return *this;
    }
    if (texture->inUse == false) {
        LOG_ERROR("Failed set storageImage name: \" {} \", the storageImage inUse == false", name);
        return *this;
    }
    if (isStatic) {
        for (auto& set : materialData->descriptorSets) {
            VkDescriptorImageInfo descriptorInfo = {
                .sampler = texture->sampler,
                .imageView = baseMipmap == 0 ? texture->imageView : texture->mipMapViews[baseMipmap],
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            };
            VkWriteDescriptorSet descriptorWrite = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = set,
                .dstBinding = binding,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .pImageInfo = &descriptorInfo,
            };
            vkUpdateDescriptorSets(vulkanRenderAPI.GetVulkanDevice()->logicalDevice,
                                   1, &descriptorWrite, 0, nullptr);
        }
    }else {
        VkDescriptorImageInfo descriptorInfo = {
            .sampler = VK_NULL_HANDLE,
            .imageView = baseMipmap == 0 ? texture->imageView : texture->mipMapViews[baseMipmap],
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        };
        VkWriteDescriptorSet descriptorWrite = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = materialData->descriptorSets[vulkanRenderAPI.GetCurrentFrame()],
            .dstBinding = binding,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo = &descriptorInfo,
        };
        vkUpdateDescriptorSets(vulkanRenderAPI.GetVulkanDevice()->logicalDevice,
                               1, &descriptorWrite, 0, nullptr);
    }
    return *this;
}

FrameWork::CompMaterial & FrameWork::CompMaterial::
SetStorageBuffer(const std::string &name, uint32_t id, bool isStatic) {
    //需要用到时在写，现在暂时无用,Comp只用来后处理
    return *this;
}


FrameWork::CompShader * FrameWork::CompMaterial::GetShader() const {
    if (CompShader::exist(shaderRef)) {
        return CompShader::Get(shaderRef);
    }
    if (shaderHandle) {
        return CompShader::Get(shaderHandle);
    }
    LOG_ERROR("the compute material shader has been destroyed");
    return nullptr;
}

void FrameWork::CompMaterial::Bind(const VkCommandBuffer &commandBuffer) const {
    uint32_t pipelineID = UINT32_MAX;
    if (CompShader::exist(shaderRef)) {
        pipelineID = CompShader::Get(shaderRef)->GetPipelineID();
    }else if (shaderHandle) {
        pipelineID = CompShader::GetPipelineID(shaderHandle);
    }else {
        LOG_ERROR("Failed to Bind Material, because it's shader has been destroyed");
        return;
    }
    auto vulkanPipeline = vulkanRenderAPI.getByIndex<FrameWork::VulkanPipeline>(pipelineID);
    auto materialData = vulkanRenderAPI.getByIndex<FrameWork::CompMaterialData>(compDataID);

    vkCmdBindDescriptorSets(
        commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vulkanPipeline->pipelineLayout, 0, 1,
        &materialData->descriptorSets[vulkanRenderAPI.currentFrame], 0, nullptr
    );
}

FrameWork::CompMaterial::CompMaterial(uint32_t shaderRef) {
    this->shaderRef = shaderRef;
    vulkanRenderAPI.CreateCompMaterialData(*this);
}
FrameWork::CompMaterial::CompMaterial(const Handle<CompShader>& shaderHandle) {
    this->shaderHandle = shaderHandle;
    vulkanRenderAPI.CreateCompMaterialData(compDataID, shaderHandle);
}

namespace FrameWork
{
    Handle<CompMaterial> CompMaterial::CreateHandle(const Handle<CompShader>& shaderHandle){
        uint32_t index = compMaterialWrappedPool.CreateResource(shaderHandle);
        Handle<CompMaterial> handle;
        handle.index = index;
        return handle;
    }
    bool CompMaterial::Bind(Handle<CompMaterial>& handle, const VkCommandBuffer& cmdBuffer) {
        if (!handle) {
            LOG_ERROR("Trying to bind a non-existent CompMaterial handle");
            return false;
        }
        auto material = compMaterialWrappedPool.GetResource(handle.index);
        if (!material) {
            LOG_ERROR("CompMaterial with index {} does not exist", handle.index);
            return false;
        }
        if (!material->shaderHandle) {
            LOG_ERROR("Failed to bind CompMaterial handle {}, shader handle is invalid", handle.index);
            return false;
        }
        auto pipelineID = CompShader::GetPipelineID(material->shaderHandle);
        auto vulkanPipeline = vulkanRenderAPI.getByIndex<FrameWork::VulkanPipeline>(pipelineID);
        auto materialData = vulkanRenderAPI.getByIndex<FrameWork::CompMaterialData>(material->compDataID);
        if (vulkanPipeline == nullptr || materialData == nullptr) {
            LOG_ERROR("Failed to bind CompMaterial handle {}, missing pipeline/material data", handle.index);
            return false;
        }
        vkCmdBindDescriptorSets(
            cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vulkanPipeline->pipelineLayout, 0, 1,
            &materialData->descriptorSets[vulkanRenderAPI.currentFrame], 0, nullptr
        );
        return true;
    }
    uint32_t& CompMaterial::GetMaterialDataID(const Handle<CompMaterial>& handle) {
        return compMaterialWrappedPool.GetResource(handle.index)->compDataID;
    }
    Handle<CompShader> CompMaterial::GetShaderHandle(const Handle<CompMaterial>& handle){
        return compMaterialWrappedPool.GetResource(handle.index)->shaderHandle;
    }
}

namespace FrameWork {
    void CompMaterial::SetTexture(const Handle<CompMaterial>& handle, const std::string& name, uint32_t id){
        auto material = compMaterialWrappedPool.GetResource(handle.index);
        if (!material) {
            LOG_ERROR("Failed to set texture for material \"{}\" , handle {} is invalid", name, handle.index);
            return;
        }
        auto shaderHandle = material->shaderHandle;
        if (!shaderHandle) {
            LOG_ERROR("Failed to set texture for material \"{}\" , shader handle is invalid", name);
            return;
        }
        auto shaderInfo = CompShader::GetInfo(shaderHandle);
        auto materialData = vulkanRenderAPI.getByIndex<FrameWork::CompMaterialData>(material->compDataID);
       uint32_t binding = -1;
       for (int i = 0; i < shaderInfo.shaderProperties.textureProperties.size(); i++) {
           if (name == shaderInfo.shaderProperties.textureProperties[i].name) {
               binding = shaderInfo.shaderProperties.textureProperties[i].binding;
               break;
           }
       }
       if (binding == -1) {
            LOG_ERROR("Can't find name : {} in shader handle {}", name, shaderHandle.index);
            return;
       }

       auto texture = vulkanRenderAPI.getByIndex<FrameWork::Texture>(id);
       if (texture == nullptr) {
           LOG_ERROR("Failed to set texture for material \"{}\", the texture is nullptr ", name);
           return ;
       }
       if (texture->inUse == false) {
           LOG_ERROR("Failed set texture name: \" {} \", the texture inUse == false", name);
           return;
       }
       for (auto& set : materialData->descriptorSets) {
           VkDescriptorImageInfo descriptorInfo = {
               .sampler = texture->sampler,
               .imageView = texture->imageView,
               .imageLayout = VK_IMAGE_LAYOUT_GENERAL
           };
           VkWriteDescriptorSet descriptorWrite = {
               .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
               .pNext = nullptr,
               .dstSet = set,
               .dstBinding = binding,
               .dstArrayElement = 0,
               .descriptorCount = 1,
               .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
               .pImageInfo = &descriptorInfo,
           };
           vkUpdateDescriptorSets(vulkanRenderAPI.GetVulkanDevice()->logicalDevice,
                                  1, &descriptorWrite, 0, nullptr);
       }
    }
    void CompMaterial::SetAttachment(const Handle<CompMaterial>& handle, const std::string& name, uint32_t id){
        auto material = compMaterialWrappedPool.GetResource(handle.index);
        if (!material) {
            LOG_ERROR("Failed to set attachment \"{}\" , handle {} is invalid", name, handle.index);
            return;
        }
        auto shaderHandle = material->shaderHandle;
        if (!shaderHandle) {
            LOG_ERROR("Failed to set attachment for material \"{}\" , shader handle is invalid", name);
            return;
        }
        auto shaderInfo = CompShader::GetInfo(shaderHandle);
        auto materialData = vulkanRenderAPI.getByIndex<FrameWork::CompMaterialData>(material->compDataID);
       uint32_t binding = -1;
       for (int i = 0; i < shaderInfo.shaderProperties.textureProperties.size(); i++) {
           if (name == shaderInfo.shaderProperties.textureProperties[i].name) {
               binding = shaderInfo.shaderProperties.textureProperties[i].binding;
               break;
           }
       }
       if (binding == -1) {
            LOG_ERROR("Can't find name : {} in shader handle {}", name, shaderHandle.index);
            return;
       }

       auto texture = vulkanRenderAPI.getByIndex<FrameWork::Texture>(id);
       if (texture == nullptr) {
           LOG_ERROR("Failed to set texture for material \"{}\", the texture is nullptr ", name);
           return;
       }
       if (texture->inUse == false) {
           LOG_ERROR("Failed set texture name: \" {} \", the texture inUse == false", name);
           return;
       }
       VkDescriptorImageInfo descriptorInfo = {
           .sampler = texture->sampler,
           .imageView = texture->imageView,
           .imageLayout = VK_IMAGE_LAYOUT_GENERAL
       };
       VkWriteDescriptorSet descriptorWrite = {
           .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
           .pNext = nullptr,
           .dstSet = materialData->descriptorSets[vulkanRenderAPI.GetCurrentFrame()],
           .dstBinding = binding,
           .dstArrayElement = 0,
           .descriptorCount = 1,
           .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
           .pImageInfo = &descriptorInfo,
       };
       vkUpdateDescriptorSets(vulkanRenderAPI.GetVulkanDevice()->logicalDevice,
                              1, &descriptorWrite, 0, nullptr);
    }
    void CompMaterial::SetStorageImage2D(const Handle<CompMaterial>& handle, const std::string& name, uint32_t id, uint32_t baseMipmap, bool isStatic){
        auto material = compMaterialWrappedPool.GetResource(handle.index);
        if (!material) {
            LOG_ERROR("Failed to set storage image \"{}\" , handle {} is invalid", name, handle.index);
            return;
        }
        auto shaderHandle = material->shaderHandle;
        if (!shaderHandle) {
            LOG_ERROR("Failed to set storage image for material \"{}\" , shader handle is invalid", name);
            return;
        }
        auto shaderInfo = CompShader::GetInfo(shaderHandle);
        auto materialData = vulkanRenderAPI.getByIndex<FrameWork::CompMaterialData>(material->compDataID);
       uint32_t binding = -1;
       for (int i = 0; i < shaderInfo.ssbos.size(); i++) {
           if (shaderInfo.ssbos[i].name == name) {
               if (shaderInfo.ssbos[i].type != StorageObjectType::Buffer) {
                   binding = shaderInfo.ssbos[i].binding;
               }
           }
       }
       if (binding == -1) {
            LOG_ERROR("Can't find storageImage name : {} in shader handle {}", name, shaderHandle.index);
            return;
       }

       auto texture = vulkanRenderAPI.getByIndex<FrameWork::Texture>(id);

       if (texture == nullptr) {
           LOG_ERROR("Failed to set storageImage for material \"{}\", the storageImage is nullptr ", name);
           return;
       }
       if (texture->inUse == false) {
           LOG_ERROR("Failed set storageImage name: \" {} \", the storageImage inUse == false", name);
           return;
       }
       if (isStatic) {
           for (auto& set : materialData->descriptorSets) {
               VkDescriptorImageInfo descriptorInfo = {
                   .sampler = texture->sampler,
                   .imageView = baseMipmap == 0 ? texture->imageView : texture->mipMapViews[baseMipmap],
                   .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
               };
               VkWriteDescriptorSet descriptorWrite = {
                   .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                   .pNext = nullptr,
                   .dstSet = set,
                   .dstBinding = binding,
                   .dstArrayElement = 0,
                   .descriptorCount = 1,
                   .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                   .pImageInfo = &descriptorInfo,
               };
               vkUpdateDescriptorSets(vulkanRenderAPI.GetVulkanDevice()->logicalDevice,
                                      1, &descriptorWrite, 0, nullptr);
           }
       }else {
           VkDescriptorImageInfo descriptorInfo = {
               .sampler = VK_NULL_HANDLE,
               .imageView = baseMipmap == 0 ? texture->imageView : texture->mipMapViews[baseMipmap],
               .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
           };
           VkWriteDescriptorSet descriptorWrite = {
               .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
               .pNext = nullptr,
               .dstSet = materialData->descriptorSets[vulkanRenderAPI.GetCurrentFrame()],
               .dstBinding = binding,
               .dstArrayElement = 0,
               .descriptorCount = 1,
               .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
               .pImageInfo = &descriptorInfo,
           };
           vkUpdateDescriptorSets(vulkanRenderAPI.GetVulkanDevice()->logicalDevice,
                                  1, &descriptorWrite, 0, nullptr);
       }
    }
    void CompMaterial::SetStorageBuffer(const Handle<CompMaterial>& handle, const std::string& name, uint32_t id, bool isStatic){
        //TODO:等到使用在添加
    }
}
