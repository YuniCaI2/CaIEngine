#include<ResourceManager.h>

#include "Logger.h"


void testLoadTexture(){
    std::string texturePath = "../../resources/Pic/doro.png";
    try {
        FrameWork::ResourceManager::GetInstance().LoadTextureAssetFromSource(texturePath);
    }catch (std::exception& e) {
        LOG_ERROR("{}",e.what());
    }
}

int main(){
    LOG.Run();

    testLoadTexture();

    LOG.Stop();
}
