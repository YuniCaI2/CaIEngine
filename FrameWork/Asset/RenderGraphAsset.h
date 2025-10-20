#pragma once

#include<string>
#include<vector>
#include "TextrueAsset.h"
#include<nlohmann/json.hpp>

enum class RenderTaskType {
    FullScreen,
    Geometry
};

NLOHMANN_JSON_SERIALIZE_ENUM(RenderTaskType,
    {
        {RenderTaskType::FullScreen, "FullScreen"},
        {RenderTaskType::Geometry, "Geometry"}
    }
    )


enum class PassShaderType {
    Graphics,
    Compute
};

NLOHMANN_JSON_SERIALIZE_ENUM(PassShaderType,
    {
        {PassShaderType::Graphics, "Graphics"},
        {PassShaderType::Compute, "Compute"}
    }
    )



struct RenderGraphPass {
    std::string passName{};
    std::string shaderTag{};
    //在使用RenderTaskType为FullScreen时绑定
    std::string materialPath{};
    RenderTaskType renderTaskType{RenderTaskType::Geometry};
    PassShaderType passShaderType{PassShaderType::Graphics};


    std::vector<std::string> passInputs;
    std::vector<std::string> passCreates;
    std::vector<std::string> passOutputs;
    std::vector<std::string> passReads;

};

//这些是帧内资源
struct TextureTransient {
    std::string passName;
    TextureImport textureInfo;
};


struct RenderGraphAsset {
    std::string name;
    std::string contextHash;
    std::filesystem::file_time_type fileTime;
    std::vector<RenderGraphPass> passes;
};

