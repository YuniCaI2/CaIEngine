//
// Created by 51092 on 2025/10/16.
//

#ifndef CAIENGINE_MODELASSET_H
#define CAIENGINE_MODELASSET_H
#include<iostream>
#include<vector>

struct ModelImport {
    bool genNormal{true};
    bool genTangent{true};
    bool flipUV{false};
    float scale{1.0f};
    //...
};

struct ModelNode {
    std::string name;
    uint32_t index; //在Model中Nodes array中的索引
    uint32_t parentIndex{UINT32_MAX};
    std::vector<uint32_t> childrenIndices{};
    std::string meshPath;
    std::string materialPath;
};

struct ModelAsset {
    ModelImport import;
    std::string name;
    std::string contentHash;
    std::filesystem::file_time_type fileTime; //shader加载时间
    uint32_t rootNode;

    //bin
    std::vector<ModelNode> nodes;
};

namespace Asset_Impl {
    struct ModelAsset_Impl {
        ModelImport import;
        std::string name;
        std::string contentHash;
        std::filesystem::file_time_type fileTime; //shader加载时间
        uint32_t rootNode;
        std::string binPath;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ModelAsset_Impl, import, name, contentHash,
        fileTime, rootNode, binPath)
}

#endif //CAIENGINE_MODELASSET_H