//
// Created by 51092 on 2025/10/16.
//

#ifndef CAIENGINE_SHADERASSET_H
#define CAIENGINE_SHADERASSET_H
#include<string>

#include "TextrueAsset.h"
#include"../PublicStruct.h"

struct ShaderPass {
    std::string passName;
    std::string sourcePath; //检测sourcePath是否修改，修改则重新编译
    std::string contentHash;
    std::filesystem::file_time_type fileTime; //shader加载时间
    FrameWork::ShaderInfo shaderInfo; //其中包括如何操作队列
    uint32_t shaderSize;
    std::unique_ptr<uint32_t[]> shaderCode; //shaderCode //存储成bin文件
};

struct TextureTransient {
    std::string passName;
    TextureImport textureInfo;
};

struct GlobalParam {
    template<class T>
    using ParamMap = std::map<std::string, T>;

    ParamMap<float> floatParams{};
    ParamMap<uint32_t> uintParams{};
    ParamMap<int> intParams{};
    ParamMap<glm::vec2> vec2Params{};
    ParamMap<glm::vec3> vec3Params{};
    ParamMap<glm::vec4> vec4Params{};

    ParamMap<std::string> Textures{}; //这里指的是TexturePath
};

struct RenderGraphPass {
    std::string passName;
    std::vector<std::string> passInputs;
    std::vector<std::string> passCreates;
    std::vector<std::string> passOutputs;
    std::vector<std::string> passReads;
};

struct ShaderRenderGraph { //维护拓扑关系
    std::unordered_map<std::string, TextureImport> transientAttachments;
    GlobalParam globalParam{}; //提前声明的静态资源
    std::unordered_map<std::string, RenderGraphPass> passes;
};

struct ShaderAsset {
    std::string name;
    ShaderRenderGraph renderGraph;
    std::unordered_map<std::string, ShaderPass> passes;
};

namespace Asset_Impl {
    struct ShaderAsset_Impl {
        std::string name;
        ShaderRenderGraph renderGraph;
        std::unordered_map<std::string, std::string> passes; //指定对应bin路径
    };
}




#endif //CAIENGINE_SHADERASSET_H