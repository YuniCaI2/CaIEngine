//
// Created by 51092 on 25-6-13.
//

#ifndef RESOURCE_H
#define RESOURCE_H
#include <string>
#include <unordered_map>
#include <assimp/scene.h>

#include "pubh.h"

#include "PublicEnum.h"
#include "PublicStruct.h"

namespace FrameWork {
    //这个类的作用是将外部的资源加载或者转为程序可用的资源
    class Resource {
    private:
        std::string resourcePath{"../resources/"};
        std::string generalShaderPath{"../resources/shaders/glsl/"};
        std::string generalModelPath{"../resources/models/"};

        std::unordered_map<std::string, TextureFullData> textureMap;

        void processNode(aiNode *node, const aiScene *scene, std::vector<MeshData>& meshes, std::string);
        FrameWork::MeshData processMesh(aiMesh *mesh, const aiScene *scene, std::string);
    public:

        Resource();
        VkShaderModule getShaderModulFromFile(VkDevice device,const std::string& fileName, VkShaderStageFlags stage) const;
        std::vector<MeshData> LoadOBJMesh(const std::string& fileName);
        std::vector<TextureFullData> LoadTextureFullDatas(aiMaterial* mat, aiTextureType type, std::string directory);
        TextureFullData LoadTextureFullData(const std::string& filePath, TextureType type);
        void ReleaseTextureFullData(const TextureFullData& textureFullData);

    };
}

#endif //RESOURCE_H
