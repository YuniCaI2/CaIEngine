//
// Created by 51092 on 2025/10/16.
//

#ifndef CAIENGINE_MODELASSET_H
#define CAIENGINE_MODELASSET_H
#include<iostream>
#include<vector>
#include"SaveTool.h"
#include "../Logger.h"
#include<nlohmann/json.hpp>

//ModelAsset 也类似结构化资源

struct ModelImport {
    bool genNormal{true};
    bool genTangent{true};
    bool flipUV{true}; //Vulkan默认导致，相对于OpenGL
    float scale{1.0f};
    //...
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
    ModelImport, genNormal, genTangent, flipUV, scale
    )


struct ModelNode {
    std::string name{};
    uint32_t index{}; //在Model中Nodes array中的索引
    uint32_t parentIndex{UINT32_MAX};
    std::vector<uint32_t> childrenIndices{};

    //两者一一对应
    std::vector<std::string> meshes;
    std::vector<std::string> materials;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ModelNode, name, index, parentIndex,
    childrenIndices, meshes, materials)



struct ModelAsset : BaseAsset {
    ModelImport import;
    // std::string name;
    // std::string sourcePath;
    // std::string contentHash;
    // std::filesystem::file_time_type fileTime; //shader加载时间
    uint32_t rootNode;

    std::vector<ModelNode> nodes;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ModelAsset, import, name, sourcePath, contentHash, fileTime, dirt, rootNode, nodes)

namespace Asset_Impl {
    struct ModelAsset_Impl : BaseAsset {
        ModelImport import;
        // std::string name;
        // std::string contentHash;
        // std::filesystem::file_time_type fileTime; //shader加载时间
        uint32_t rootNode;
        std::string binPath;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ModelAsset_Impl, import, name, sourcePath, contentHash,
        fileTime, dirt, rootNode, binPath)


    inline void SaveModelNode(std::ofstream& file, const ModelNode& modelNode) {
        uint32_t nameSize = static_cast<uint32_t>(modelNode.name.size());
        file.write(reinterpret_cast<const char*>(&nameSize), sizeof(uint32_t));
        file.write(modelNode.name.data(), nameSize);

        file.write(reinterpret_cast<const char*>(&modelNode.index), sizeof(uint32_t));
        file.write(reinterpret_cast<const char*>(&modelNode.parentIndex), sizeof(uint32_t));

        uint32_t childCount = static_cast<uint32_t>(modelNode.childrenIndices.size());
        file.write(reinterpret_cast<const char*>(&childCount), sizeof(uint32_t));
        if (childCount > 0)
            file.write(reinterpret_cast<const char*>(modelNode.childrenIndices.data()), sizeof(uint32_t) * childCount);


        uint32_t meshSize = static_cast<uint32_t>(modelNode.meshes.size());
        file.write(reinterpret_cast<const char*>(&meshSize), sizeof(uint32_t));
        for (uint32_t i = 0; i < meshSize; i++) {
            AssetHelper::SaveString(file, modelNode.meshes[i]);
        }

        uint32_t materialSize = static_cast<uint32_t>(modelNode.materials.size());
        file.write(reinterpret_cast<const char*>(&materialSize), sizeof(uint32_t));
        for (uint32_t i = 0; i < materialSize; i++) {
            AssetHelper::SaveString(file, modelNode.materials[i]);
        }
    }


    inline void LoadModelNode(std::ifstream& file, ModelNode& modelNode) {
        uint32_t nameSize = 0;
        file.read(reinterpret_cast<char*>(&nameSize), sizeof(uint32_t));
        modelNode.name.resize(nameSize);
        file.read(modelNode.name.data(), nameSize);

        file.read(reinterpret_cast<char*>(&modelNode.index), sizeof(uint32_t));
        file.read(reinterpret_cast<char*>(&modelNode.parentIndex), sizeof(uint32_t));

        uint32_t childCount = 0;
        file.read(reinterpret_cast<char*>(&childCount), sizeof(uint32_t));
        modelNode.childrenIndices.resize(childCount);
        if (childCount > 0)
            file.read(reinterpret_cast<char*>(modelNode.childrenIndices.data()), sizeof(uint32_t) * childCount);

        uint32_t meshSize = 0;
        file.read(reinterpret_cast<char*>(&meshSize), sizeof(uint32_t));
        modelNode.meshes.resize(meshSize);
        for (uint32_t i = 0; i < meshSize; i++) {
            AssetHelper::LoadString(file, modelNode.meshes[i]);
        }

        uint32_t materialSize = 0;
        file.read(reinterpret_cast<char*>(&materialSize), sizeof(uint32_t));
        modelNode.materials.resize(materialSize);
        for (uint32_t i = 0; i < materialSize; i++) {
            AssetHelper::LoadString(file, modelNode.materials[i]);
        }
    }


}
inline void SaveModelBin(const ModelAsset& modelAsset, const std::string& binPath) {
    uint32_t size = modelAsset.nodes.size();
    std::ofstream binFile(binPath);

    binFile.write(reinterpret_cast<const char*>(&size), sizeof(uint32_t));
    for (uint32_t i = 0; i < size; i++) {
        Asset_Impl::SaveModelNode(binFile, modelAsset.nodes[i]);
    }
}

inline void LoadModelBin(ModelAsset& modelAsset, const std::string& binPath) {
    uint32_t size = 0;
    std::ifstream binFile(binPath);
    if (!binFile.is_open()) {
        LOG_ERROR("Can't open file %s", binPath.c_str());
        throw std::runtime_error("Can't open file : " + binPath);
    }
    binFile.read(reinterpret_cast<char*>(&size), sizeof(uint32_t));
    modelAsset.nodes.resize(size);
    for (uint32_t i = 0; i < size; i++) {
        Asset_Impl::LoadModelNode(binFile, modelAsset.nodes[i]);
    }
}

#endif //CAIENGINE_MODELASSET_H
