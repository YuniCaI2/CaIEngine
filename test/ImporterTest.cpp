#include<Importer.h>

#include "Logger.h"


void testLoadTexture(){
    std::string texturePath = "../../resources/Pic/doro.png";
    auto id = FrameWork::Importer::GetInstance().LoadTextureSource(texturePath);
    if (id.has_value()) {
        LOG_TRACE("id: {}", id.value());

        //打印JSON
        std::ifstream file(texturePath + ".meta");
        nlohmann::json json = nlohmann::json::parse(file);
        std::cout << json << std::endl;
    }else {
        LOG_ERROR("id: {}", id.error().msg);
    }
}

int main(){
    LOG.Run();
    testLoadTexture();

    LOG.Stop();
}
