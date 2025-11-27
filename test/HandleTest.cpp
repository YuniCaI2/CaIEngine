//
// Created by 51092 on 2025/11/18.
//
#include<vulkanFrameWork.h>
#include"Logger.h"
#include "PublicStruct.h"


int main() {
	LOG.Run();
    FrameWork::Handle<FrameWork::Texture> textureHandle = vulkanRenderAPI.CreateTextureHandleTest();
    auto *ptr = vulkanRenderAPI.GetResource(textureHandle);
    {
        auto textureCopy = textureHandle;
        LOG_TRACE("Ref: {}", vulkanRenderAPI.GetRefNum<FrameWork::Texture>(textureCopy.index));
    }
    LOG_TRACE("Ref: {}", vulkanRenderAPI.GetRefNum<FrameWork::Texture>(textureHandle.index));
    LOG_TRACE("index: {}", textureHandle.index);

	LOG.Stop();

}
