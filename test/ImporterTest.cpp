#include<ResourceManager.h>

#include "Logger.h"


void testLoadTexture(){
    std::string texturePath = "../../resources/Pic/doro.png";
    try {
        FrameWork::ResourceManager::GetInstance().LoadTextureAssetFromSourceAsync(texturePath);
    }catch (std::exception& e) {
        LOG_ERROR("{}",e.what());
    }
}

void testCreateShader() {
    FrameWork::ResourceManager::GetInstance().CreateShaderAssetAsync("TestCreateShader");
}

void TestLoadModelAsset() {
    std::string modelPath = "../../resources/models/cocona/cocona.obj";
    try {
        auto& resourceManager = FrameWork::ResourceManager::GetInstance();
        auto n = resourceManager.LoadModelAssetFromSourceAsync(modelPath);
        n.wait();
    } catch (std::exception& e) {
        LOG_ERROR("{}",e.what());
    }

    nlohmann::json j;
}


struct Asset : BaseAsset {
    uint32_t test{1};
};
SERIALIZE_ASSET(Asset, test)

int main(){
    ThreadPool::GetInstance();
    LOG.Run();
    // Asset* asset = new Asset();
    // nlohmann::json json = *asset;
    // std::cout << std::setw(4) << json << std::endl;
    TestLoadModelAsset();
    LOG.Stop();
}
