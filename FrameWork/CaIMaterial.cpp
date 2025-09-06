//
// Created by cai on 2025/9/4.
//

#include "CaIMaterial.h"

#include <set>

#include "vulkanFrameWork.h"

FrameWork::CaIMaterial::CaIMaterial(std::weak_ptr<CaIShader> shader) {
    this->shaderRef = std::move(shader);
    vulkanRenderAPI.CreateMaterialData(*this);
}

FrameWork::CaIMaterial::~CaIMaterial() {
    vulkanRenderAPI.DeleteMaterialData(dataID);
}

void FrameWork::CaIMaterial::SetTexture(const std::string &name,uint32_t id) const {
    //Update DescriptorSet
    if (shaderRef.expired()) {
        ERROR("Failed to set texture for material \"{}\" ", name);
        return;
    }
    auto shaderInfo = shaderRef.lock()->GetShaderInfo();
    auto materialData = vulkanRenderAPI.getByIndex<FrameWork::MaterialData>(id);
    uint32_t binding = -1;
    for (auto& texProperty : shaderInfo.vertProperties.textureProperties) {
        if (name == texProperty.name) {
            binding = id;
            break;
        }
    }

    if (binding == -1)
        for (auto& texProperty : shaderInfo.fragProperties.textureProperties) {
            if (name == texProperty.name) {
                binding = id;
                break;
            }
        }

    auto texture = vulkanRenderAPI.getByIndex<FrameWork::Texture>(id);
    if (texture == nullptr) {
        ERROR("Failed to set texture for material \"{}\", the texture is nullptr ", name);
        return;
    }
    if (texture->inUse == false) {
        ERROR("Failed set texture name: \" {} \", the texture inUse == false", name);
        return;
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
}

void FrameWork::CaIMaterial::SetAttachment(const std::string &name, uint32_t id) const {
    //Update DescriptorSet
    if (shaderRef.expired()) {
        ERROR("Failed to set texture for material \"{}\" ", name);
        return;
    }
    auto shaderInfo = shaderRef.lock()->GetShaderInfo();
    auto materialData = vulkanRenderAPI.getByIndex<FrameWork::MaterialData>(id);
    uint32_t binding = -1;
    for (auto& texProperty : shaderInfo.vertProperties.textureProperties) {
        if (name == texProperty.name) {
            binding = id;
            break;
        }
    }

    if (binding == -1)
        for (auto& texProperty : shaderInfo.fragProperties.textureProperties) {
            if (name == texProperty.name) {
                binding = id;
                break;
            }
        }

    auto attachment = vulkanRenderAPI.getByIndex<FrameWork::VulkanAttachment>(id);
    if (attachment == nullptr) {
        ERROR("Failed to set texture for material \"{}\", the texture is nullptr ", name);
        return;
    }
    if (attachment->inUse == false) {
        ERROR("Failed set texture name: \" {} \", the texture inUse == false", name);
        return;
    }
    for (int i = 0; i < materialData->descriptorSets.size(); i++) {
        auto texture = vulkanRenderAPI.getByIndex<FrameWork::Texture>(attachment->attachmentsArray[i]);
        VkDescriptorImageInfo descriptorInfo = {
            .sampler = texture->sampler,
            .imageView = texture->imageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };
        VkWriteDescriptorSet descriptorWrite = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = materialData->descriptorSets[i],
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

std::weak_ptr<FrameWork::CaIShader> FrameWork::CaIMaterial::GetShader() const{
    return shaderRef;
}

void FrameWork::CaIMaterial::Bind(const VkCommandBuffer &cmdBuffer) const {
    if (shaderRef.expired()) {
        ERROR("Failed to Bind Material, because it's shader has been destroyed");
    }
    auto vulkanPipeline = vulkanRenderAPI.getByIndex<FrameWork::VulkanPipeline>(shaderRef.lock()->GetPipelineID());
    auto materialData = vulkanRenderAPI.getByIndex<FrameWork::MaterialData>(dataID);
    vkCmdBindDescriptorSets(
        cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline->pipelineLayout, 0, 1,
        &materialData->descriptorSets[vulkanRenderAPI.currentFrame], 0, nullptr
        );
}
