//
// Created by 51092 on 25-6-13.
//

#ifndef RESOURCE_H
#define RESOURCE_H
#include <string>
#include <unordered_map>
#include <filesystem>
#include <assimp/scene.h>

#include "pubh.h"

#include "PublicEnum.h"
#include "PublicStruct.h"
#include <map>

namespace FrameWork {
    //这个类的作用是将外部的资源加载或者转为程序可用的资源
    class Resource {
    private:
        Resource();

        std::string resourcePath{"../resources/"};
        std::string generalShaderPath{"../resources/shaders/glsl/"};
        std::string generalModelPath{"../resources/models/"};
        std::string shaderTimeCachePath{"../resources/shaders/shaderTimeCache.bin"};

        std::unordered_map<std::string, TextureFullData> textureMap;
        using ShaderTimeCache =  std::map<std::string, std::filesystem::file_time_type>;
        mutable ShaderTimeCache shaderTimeCache;

        void processNode(aiNode *node, const aiScene *scene, std::vector<MeshData>& meshes, ModelType modelType, std::string, TextureTypeFlags textureFlags);
        FrameWork::MeshData processMesh(aiMesh *mesh, ModelType modelType, const aiScene *scene, std::string, TextureTypeFlags textureFlags);
        TextureFullData CreateDefaultTexture(TextureTypeFlagBits textureFlagBits);

        void SaveCache(const std::string& filePath) const;
        void LoadShaderCache() const;

        void CompileShader(const std::string& filepath) const;
        void CompileShaderModify() const;
    public:


        VkShaderModule getShaderModulFromFile(VkDevice device,const std::string& fileName, VkShaderStageFlags stage) const;
        std::vector<MeshData> LoadMesh(const std::string& fileName, ModelType modelType, TextureTypeFlags textureFlags, float scale = 1.0f);
        std::vector<TextureFullData> LoadTextureFullDatas(aiMaterial* mat, const aiScene* scene,aiTextureType type, std::string directory);
        TextureFullData LoadTextureFullData(const std::string& filePath, TextureTypeFlagBits type);
        void ReleaseTextureFullData(const TextureFullData& textureFullData);


        static Resource& GetInstance();
    };
}

#endif //RESOURCE_H
