//
// Created by cai on 2025/9/23.
//

#include "CompMaterial.h"

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

FrameWork::CompMaterial * FrameWork::CompMaterial::Destroy(const uint32_t &id) {
    if (id >= compMaterialPool.size()) {
        LOG_ERROR("Comp Material ID: {}  out of range", id);
        return nullptr;
    }
    return compMaterialPool[id];
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
        vulkanRenderAPI.DeleteMaterialData(compDataID);
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
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
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
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
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
    for (int i = 0; i < shaderInfo.shaderProperties.textureProperties.size(); i++) {
        if (name == shaderInfo.shaderProperties.textureProperties[i].name) {
            if (shaderInfo.ssbos[i].type == StorageObjectType::Image2D)
                binding = shaderInfo.ssbos[i].binding;
            break;
        }
    }
    if (binding == -1) {
        LOG_ERROR("Can't find storageImage name : {} in shader: {}", name, shaderRef);
        return *this;
    }

    auto texture = vulkanRenderAPI.getByIndex<FrameWork::Texture>(id);
    if (baseMipmap >= texture->image.mipLevels) {
        LOG_ERROR("The Base Mimap: {} > image mipmap Level : {}", baseMipmap, texture->image.mipLevels);
        return *this;
    }
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
                .imageView = texture->mipMapViews[baseMipmap],
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
            .sampler = texture->sampler,
            .imageView = texture->mipMapViews[baseMipmap],
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
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
    return nullptr;
}

void FrameWork::CompMaterial::Bind(const VkCommandBuffer &commandBuffer) const {
    if (!CompShader::exist(shaderRef)) {
        LOG_ERROR("Failed to Bind Material, because it's shader has been destroyed");
    }
    auto vulkanPipeline = vulkanRenderAPI.getByIndex<FrameWork::VulkanPipeline>(
        CompShader::Get(shaderRef)->GetPipelineID());
    auto materialData = vulkanRenderAPI.getByIndex<FrameWork::CompMaterialData>(compDataID);

    vkCmdBindDescriptorSets(
        commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline->pipelineLayout, 0, 1,
        &materialData->descriptorSets[vulkanRenderAPI.currentFrame], 0, nullptr
    );
}

FrameWork::CompMaterial::CompMaterial(uint32_t shaderRef) {
    this->shaderRef = shaderRef;
    vulkanRenderAPI.CreateCompMaterialData(*this);
}
