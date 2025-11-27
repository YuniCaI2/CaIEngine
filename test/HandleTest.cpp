//
// Created by 51092 on 2025/11/18.
//
#include<vulkanFrameWork.h>
#include"Logger.h"


int main() {
	LOG.Run();
    auto textureHandle = vulkanRenderAPI.CreateTextureHandleTest();
    auto *ptr = vulkanRenderAPI.GetResource(textureHandle);
    auto textureCopy = textureHandle;
    LOG_TRACE("Ref: {}", vulkanRenderAPI.GetRefNum<FrameWork::Texture>(textureCopy.index));
    LOG_TRACE("index: {}", textureHandle.index);

	LOG.Stop();

}
