//
// Created by cai on 2025/9/4.
//

#include "CaIMaterial.h"
#include "Logger.h"
#include "vulkanFrameWork.h"


FrameWork::CaIMaterial::CaIMaterial(const Handle<CaIShader> &shaderHandle) {
    this->shaderHandle = shaderHandle;
    vulkanRenderAPI.CreateMaterialData(dataID, shaderHandle);
}

FrameWork::CaIMaterial::~CaIMaterial() {
    if (dataID != UINT32_MAX)
        vulkanRenderAPI.DeleteMaterialData(dataID);
}

void FrameWork::CaIMaterial::SetTexture(const Handle<CaIMaterial>& handle, const std::string& name, uint32_t id){
    auto material = CaIMaterial::caiMaterialWrappedPool.GetResource(handle.index);
    auto shaderHandle = material->shaderHandle;
    auto dataID = material->dataID;
    //Update DescriptorSet
    if (!shaderHandle) {
        LOG_ERROR("Failed to set texture for material \"{}\" ", name);
        return;
    }
    auto shaderInfo = CaIShader::GetInfo(shaderHandle);
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
        return ;
    }
    if (texture->inUse == false) {
        LOG_ERROR("Failed set texture name: \" {} \", the texture inUse == false", name);
        return ;
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


}
void FrameWork::CaIMaterial::SetAttachment(const Handle<CaIMaterial>& handle, const std::string& name, uint32_t id){
    auto material = CaIMaterial::caiMaterialWrappedPool.GetResource(handle.index);
    auto shaderHandle = material->shaderHandle;
    auto dataID = material->dataID;
    if (!shaderHandle) {
        LOG_ERROR("Failed to set texture for material \"{}\" ", name);
        return;
    }
    auto shaderInfo = CaIShader::GetInfo(shaderHandle);
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
        return;
    }
    if (texture->inUse == false) {
        LOG_ERROR("Failed set texture name: \" {} \", the texture inUse == false", name);
        return;
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

}

namespace FrameWork
{
    Handle<CaIMaterial> CaIMaterial::CreateHandle(const Handle<CaIShader>& shaderHandle){
        uint32_t index = caiMaterialWrappedPool.CreateResource(shaderHandle);
        Handle<CaIMaterial> handle;
        handle.index = index;
        return handle;
    }
    bool CaIMaterial::Bind(Handle<CaIMaterial>& handle, const VkCommandBuffer& cmdBuffer){
        if(!handle){
            LOG_ERROR("Trying to bind a non-existent CaIShader handle");
            return false;
        }
        auto material = CaIMaterial::caiMaterialWrappedPool.GetResource(handle.index);
        if (material) {
            auto vulkanPipeline = vulkanRenderAPI.getByIndex<FrameWork::VulkanPipeline>(
                CaIShader::GetPipelineID(GetShaderHandle(handle)));
            auto materialData = vulkanRenderAPI.getByIndex<FrameWork::MaterialData>(GetMaterialDataID(handle));
            vkCmdBindDescriptorSets(
                cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline->pipelineLayout, 0, 1,
                &materialData->descriptorSets[vulkanRenderAPI.currentFrame], 0, nullptr
            );
        } else {
            LOG_ERROR("CaIShader with index {} does not exist", handle.index);
            return false;
        }
        return true;
    }
    uint32_t& CaIMaterial::GetMaterialDataID(const Handle<CaIMaterial>& handle){
        auto material = CaIMaterial::caiMaterialWrappedPool.GetResource(handle.index);
        return material->dataID;
    }
    Handle<CaIShader> CaIMaterial::GetShaderHandle(const Handle<CaIMaterial>& handle){
        return CaIMaterial::caiMaterialWrappedPool.GetResource(handle.index)->shaderHandle;
    }
}
