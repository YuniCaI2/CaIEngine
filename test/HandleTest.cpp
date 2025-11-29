//
// Created by 51092 on 2025/11/18.
//
#include<vulkanFrameWork.h>
#include"Logger.h"
#include "CaIShader.h"


int main() {
	LOG.Run();
    vulkanRenderAPI.initVulkan();
    {
        auto handle = FrameWork::CaIShader::CreateHandle("../resources/CaIShaders/BaseScene/BaseScene.caishader", VK_FORMAT_R8G8B8A8_SRGB);
        LOG_TRACE("Ref : {}", FrameWork::CaIShader::GetRef(handle.index));
        auto copyHandle = handle;
        LOG_TRACE("Ref : {}", FrameWork::CaIShader::GetRef(handle.index));
        {
            auto copyHandle = handle;
            LOG_TRACE("Ref : {}", FrameWork::CaIShader::GetRef(handle.index));
        }
        LOG_TRACE("Ref : {}", FrameWork::CaIShader::GetRef(handle.index));
    }
    vulkanRenderAPI.DestroyAll();


	LOG.Stop();

}
