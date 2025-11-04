//
// Created by 51092 on 2025/10/16.
//

#ifndef CAIENGINE_SHADERASSET_H
#define CAIENGINE_SHADERASSET_H
#include <fstream>
#include <memory>
#include<string>
#include"../PublicStruct.h"
#include<nlohmann/json.hpp>

//Graphics Shader
struct ShaderPass : BaseAsset {
    std::string shaderTag;

    FrameWork::ShaderInfo shaderInfo{}; //其中包括如何操作队列
    uint32_t vertShaderSize{};
    std::unique_ptr<uint32_t[]> vertShaderCode{}; //shaderCode //存储成bin文件
    uint32_t fragShaderSize{};
    std::unique_ptr<uint32_t[]> fragShaderCode{};
};

struct ShaderAsset : BaseAsset {
    std::unordered_map<std::string, std::string> passes; //对应shaderTag To Shader Tag
    //Pass 需要能够共享
};

SERIALIZE_ASSET(ShaderAsset, passes)



//用户操作的资源
struct ShaderSource {
    std::string name;
    //ShaderTag To CaIShaderSource
    std::unordered_map<std::string, std::string> passes; //后一个str为ShaderPass sourcePath, 也就是caishader Path，不是jsonPath
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ShaderSource, name, passes)


namespace Asset_Impl {
    struct ShaderPass_Impl : BaseAsset {
        std::string shaderTag;

        FrameWork::ShaderInfo shaderInfo; //其中包括如何操作队列
        uint32_t vertShaderSize{};
        std::string vertBinPath{};
        uint32_t fragShaderSize{};
        std::string fragBinPath{};
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ShaderPass_Impl, name, shaderTag, sourcePath, contentHash, fileTime,  shaderInfo, vertShaderSize, vertBinPath, fragShaderSize, fragBinPath)
}

inline static void SaveShaderCodeBin(const std::string& path, const uint32_t* shaderCode, uint32_t shaderSize){
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(shaderCode), shaderSize);
}

//默认尺寸没有改变
inline static std::unique_ptr<uint32_t[]> LoadShaderCodeBin(const std::string& path, uint32_t shaderSize){
    std::ifstream file(path, std::ios::binary);
    std::unique_ptr<uint32_t[]> shaderCode = std::make_unique<uint32_t[]>(shaderSize);
    if (file.is_open()){
        file.read(reinterpret_cast<char*>(shaderCode.get()), shaderSize);
    }else {
        throw std::runtime_error("Could not open file to load Shader Bin" + path);
    }
    return shaderCode;
}




#endif //CAIENGINE_SHADERASSET_H
