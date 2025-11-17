//
// Created by 51092 on 2025/11/16.
//

#include "PublicStruct.h"
#include "vulkanFrameWork.h"

namespace FrameWork {
    //TODO:改成句柄,而不是直接使用索引
    void VulkanModelData::Destroy(VkDevice device) {
        for (auto& texs : textures_) {
            for (auto& [_, tex] : texs) {
                auto texturePtr = vulkanRenderAPI.GetResource(tex);
                if (texturePtr) {
                    texturePtr->Destroy(vulkanRenderAPI.GetVulkanDevice()->logicalDevice);
                }
            }
        }
    }

}
