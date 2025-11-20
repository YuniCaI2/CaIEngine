//
// Created by 51092 on 2025/11/18.
//
#include<vulkanFrameWork.h>
#include"Logger.h"


int main() {
	LOG.Run();
	auto textureHandle = vulkanRenderAPI.CreateResource<FrameWork::Texture>();
	auto *ptr = vulkanRenderAPI.GetResource(textureHandle);
    vulkanRenderAPI.DeleteResource(textureHandle);
	auto textureHandleRef = textureHandle;

    //保证资源释放
    vulkanRenderAPI.CheckDelete();
    vulkanRenderAPI.CheckDelete();
    vulkanRenderAPI.CheckDelete();
    auto newTextureHandle = vulkanRenderAPI.CreateResource<FrameWork::Texture>();
	auto firstText = vulkanRenderAPI.GetResource(textureHandleRef);
	LOG_TRACE("Show the generation: {}", textureHandle.generation);
    LOG_TRACE("Show the next index: {}", *newTextureHandle.index);
	LOG.Stop();

}
