//
// Created by 51092 on 2025/10/16.
//

#ifndef CAIENGINE_MESHASSET_H
#define CAIENGINE_MESHASSET_H
#include<string>
#include<vector>
#include<glm/glm.hpp>
#include<filesystem>

#include "../Logger.h"

struct VertexData {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec2 texCoord;
};

//source Path 是虚拟地址，用来存入表中，来代替没有GUID的缺陷

struct MeshAsset : BaseAsset { //理应上Material和Mesh不耦合
    // std::string name;
    // std::string contentHash;
    // std::string sourcePath;
    // std::filesystem::file_time_type fileTime; //shader加载时间

    //bin
    std::vector<VertexData> vertices;
    std::vector<uint32_t> indices;

};

namespace Asset_Impl {
    struct MeshAsset_Impl : BaseAsset {
        // std::string name;
        // std::string contentHash;
        // std::string sourcePath;
        // std::filesystem::file_time_type fileTime; //shader加载时间
        std::string binPath;//这里一并记录，内存布局尺寸+ vertexData + indices
    };

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MeshAsset_Impl, name, contentHash, sourcePath, fileTime, dirt, binPath)

}

inline void SaveMeshBin(const MeshAsset& meshAsset, std::string binPath) {
    if (meshAsset.vertices.size() != meshAsset.indices.size()) {
        LOG_ERROR("The number of vertices does not match the number of indices");
        throw std::runtime_error("The number of vertices does not match the number of indices");
    }
    uint32_t size = meshAsset.vertices.size();
    std::ofstream os(binPath, std::ios::binary);
    os.write(reinterpret_cast<const char*>(&size), sizeof(size));
    os.write(reinterpret_cast<const char*>(meshAsset.vertices.data()), sizeof(VertexData) * size);
    os.write(reinterpret_cast<const char*>(meshAsset.indices.data()), sizeof(uint32_t) * size);
}

inline void LoadMeshBin(MeshAsset& asset, std::string binPath) {
    std::ifstream is(binPath, std::ios::binary);
    if (!is.is_open()) {
        LOG_ERROR("The file: {} was not opened", binPath);
        throw std::runtime_error("The file: " + binPath + " was not opened");
    }
    //从内存加载
    uint32_t size;
    is.read(reinterpret_cast<char*>(&size), sizeof(size));
    asset.vertices.resize(size);
    is.read(reinterpret_cast<char*>(asset.vertices.data()), sizeof(VertexData) * size);
    asset.indices.resize(size);
    is.read(reinterpret_cast<char*>(asset.indices.data()), sizeof(uint32_t) * size);
    is.close();
}

#endif //CAIENGINE_MESHASSET_H
