//
// Created by cai on 2025/9/4.
//

#include "CaIMaterial.h"
#include "vulkanFrameWork.h"

FrameWork::CaIMaterial *FrameWork::CaIMaterial::Create(uint32_t &id, uint32_t shaderRef) {
    for (int i = 0; i < caiMaterialPools.size(); i++) {
        if (caiMaterialPools[i] == nullptr) {
            id = i;
            caiMaterialPools[i] = new FrameWork::CaIMaterial(shaderRef);
            return caiMaterialPools[i];
        }
    }
    id = caiMaterialPools.size();
    caiMaterialPools.push_back(new CaIMaterial(shaderRef));
    return caiMaterialPools.back();
}

void FrameWork::CaIMaterial::Destroy(uint32_t &id) {
    if (caiMaterialPools.size() <= id || caiMaterialPools[id] == nullptr) {
        LOG_WARNING("shaderRef: {}  is not existed", id);
        return;
    }
    delete caiMaterialPools[id];
}

FrameWork::CaIMaterial *FrameWork::CaIMaterial::Get(uint32_t id) {
    if (id >= caiMaterialPools.size() && caiMaterialPools[id] == nullptr) {
        LOG_ERROR("CaI Material ID: {}  is not existed", id);
        return nullptr;
    }
    return caiMaterialPools[id];
}

void FrameWork::CaIMaterial::DestroyAll() {
    for (auto &m: caiMaterialPools) {
        delete m;
        m = nullptr;
    }
}

bool FrameWork::CaIMaterial::exist(uint32_t id) {
    if (id >= caiMaterialPools.size() && caiMaterialPools[id] == nullptr) {
        return false;
    }
    return true;
}

FrameWork::CaIMaterial::CaIMaterial(uint32_t shaderRef) {
    this->shaderRef = shaderRef;
    vulkanRenderAPI.CreateMaterialData(*this);
}

FrameWork::CaIMaterial::~CaIMaterial() {
    if (dataID != UINT32_MAX)
        vulkanRenderAPI.DeleteMaterialData(dataID);
}

FrameWork::CaIMaterial &FrameWork::CaIMaterial::SetTexture(const std::string &name, uint32_t id)  {
    //Update DescriptorSet
    if (!CaIShader::exist(shaderRef)) {
        LOG_ERROR("Failed to set texture for material \"{}\" ", name);
        return *this;
    }
    auto shaderInfo = CaIShader::Get(shaderRef)->GetShaderInfo();
    auto materialData = vulkanRenderAPI.getByIndex<FrameWork::MaterialData>(dataID);
    uint32_t binding = -1;
    for (int i = 0; i < shaderInfo.vertProperties.textureProperties.size(); i++) {
        if (name == shaderInfo.vertProperties.textureProperties[i].name) {
            binding = shaderInfo.vertProperties.textureProperties[i].binding;
            break;
        }
    }

    if (binding == -1)
        for (int i = 0; i < shaderInfo.fragProperties.textureProperties.size(); i++) {
            if (name == shaderInfo.fragProperties.textureProperties[i].name) {
                binding = shaderInfo.fragProperties.textureProperties[i].binding;
                break;
            }
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
    for (auto &set: materialData->descriptorSets) {
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

FrameWork::CaIMaterial& FrameWork::CaIMaterial::SetAttachment(const std::string &name, uint32_t id) {
    //Update DescriptorSet
    if (!CaIShader::exist(shaderRef)) {
        LOG_ERROR("Failed to set texture for material \"{}\" ", name);
        return *this;
    }
    auto shaderInfo = CaIShader::Get(shaderRef)->GetShaderInfo();
    auto materialData = vulkanRenderAPI.getByIndex<FrameWork::MaterialData>(dataID);
    uint32_t binding = -1;
    for (int i = 0; i < shaderInfo.vertProperties.textureProperties.size(); i++) {
        if (name == shaderInfo.vertProperties.textureProperties[i].name) {
            binding = shaderInfo.vertProperties.textureProperties[i].binding;
            break;
        }
    }

    if (binding == -1)
        for (int i = 0; i < shaderInfo.fragProperties.textureProperties.size(); i++) {
            if (name == shaderInfo.fragProperties.textureProperties[i].name) {
                binding = shaderInfo.fragProperties.textureProperties[i].binding;
                break;
            }
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

FrameWork::CaIShader *FrameWork::CaIMaterial::GetShader() const {
    if (CaIShader::exist(shaderRef)) {
        return CaIShader::Get(shaderRef);
    }
    LOG_ERROR("the material shader has been destroyed");
    return nullptr;
}

uint32_t FrameWork::CaIMaterial::GetShaderID() const {
    return shaderRef;
}

void FrameWork::CaIMaterial::Bind(const VkCommandBuffer &cmdBuffer) const {
    if (!CaIShader::exist(shaderRef)) {
        LOG_ERROR("Failed to Bind Material, because it's shader has been destroyed");
    }
    auto vulkanPipeline = vulkanRenderAPI.getByIndex<FrameWork::VulkanPipeline>(
        CaIShader::Get(shaderRef)->GetPipelineID());
    auto materialData = vulkanRenderAPI.getByIndex<FrameWork::MaterialData>(dataID);
    vkCmdBindDescriptorSets(
        cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline->pipelineLayout, 0, 1,
        &materialData->descriptorSets[vulkanRenderAPI.currentFrame], 0, nullptr
    );
}
