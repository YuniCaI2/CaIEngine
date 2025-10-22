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

void testLoadShader() {
    std::string shaderTestPath = "../../resources/test/testShader.json";
    try {
        FrameWork::ResourceManager::GetInstance().LoadShaderAssetFromSource(shaderTestPath);
    }catch (std::exception& e) {
        LOG_ERROR("{}",e.what());
    }
}

void testLoadMaterial() {
    std::string materialTestPath = "../../resources/Materials/testMaterial.json";
    try {
        FrameWork::ResourceManager::GetInstance().LoadMaterialAssetFromSource(materialTestPath);
    }catch (std::exception& e) {
        LOG_ERROR("{}",e.what());
    }
}

int main(){
    LOG.Run();

    // testLoadTexture();
    // testLoadShader();
    testLoadMaterial();

    LOG.Stop();
}
