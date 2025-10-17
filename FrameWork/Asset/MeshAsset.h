//
// Created by 51092 on 2025/10/16.
//

#ifndef CAIENGINE_MESHASSET_H
#define CAIENGINE_MESHASSET_H
#include<string>
#include<vector>
#include<glm/glm.hpp>

struct VertexData {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec2 texCoord;
};

struct MeshAsset { //理应上Material和Mesh不耦合
    std::string name;

    std::string contentHash;
    std::filesystem::file_time_type fileTime; //shader加载时间
    //bin
    std::vector<VertexData> vertices;
    std::vector<uint32_t> indices;
};

namespace Asset_Impl {
    struct MeshAsset_Impl {
        std::string name;
        std::string contentHash;
        std::filesystem::file_time_type fileTime; //shader加载时间
        std::string binPath;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MeshAsset_Impl, name, contentHash, fileTime, binPath)
}

#endif //CAIENGINE_MESHASSET_H