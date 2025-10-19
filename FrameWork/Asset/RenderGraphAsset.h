#pragma once

#include<string>
#include<vector>
#include "TextrueAsset.h"
#include<unordered_map>

enum class RenderTaskType {

};
enum class PassPipeType {
    Graphics,
    Compute
};

struct RenderGraphPass {
    std::string passName;
    std::string shaderTag;

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
    RenderGraphPass renderPasses;
};
