//
// Created by 51092 on 2025/11/18.
//
#include<vulkanFrameWork.h>
#include"Logger.h"


int main() {
	LOG.Run();
	auto textureHandle = vulkanRenderAPI.CreateResource<FrameWork::Texture>();
	auto *ptr = vulkanRenderAPI.GetResource(textureHandle);
    auto firstHandleRef = textureHandle;
	LOG_TRACE("Show the generation: {}", textureHandle.generation);
    vulkanRenderAPI.DeleteResource(textureHandle);

    //保证资源释放
    vulkanRenderAPI.CheckDelete();
    vulkanRenderAPI.CheckDelete();
    vulkanRenderAPI.CheckDelete();
    auto newTextureHandle = vulkanRenderAPI.CreateResource<FrameWork::Texture>();
    LOG_TRACE("Show the next generation: {}", newTextureHandle.generation);
	LOG.Stop();

}
