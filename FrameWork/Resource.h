//
// Created by 51092 on 25-6-13.
//

#ifndef RESOURCE_H
#define RESOURCE_H
#include <string>
#include <unordered_map>
#include <filesystem>
#include <future>
#include <assimp/scene.h>

#include "pubh.h"

#include "PublicEnum.h"
#include "PublicStruct.h"
#include <map>
#include <expected>
namespace FrameWork {
    //这个类的作用是将外部的资源加载或者转为程序可用的资源
    using ShaderModulePackages = std::vector<std::pair<VkShaderStageFlagBits, VkShaderModule>>;
    class Resource {
    private:
        Resource();



        std::unordered_map<std::string, TextureFullData> textureMap;
        using ShaderTimeCache =  std::map<std::string, std::filesystem::file_time_type>;
        mutable ShaderTimeCache shaderTimeCache;
        //记录自己的shader改变情况
        mutable ShaderTimeCache caiShaderTimeCache;
        mutable std::mutex caiShaderTimeCacheMutex;

        void processNode(aiNode *node, const aiScene *scene, std::vector<MeshData>& meshes, ModelType modelType, std::string, TextureTypeFlags textureFlags);
        FrameWork::MeshData processMesh(aiMesh *mesh, ModelType modelType, const aiScene *scene, std::string, TextureTypeFlags textureFlags);
        TextureFullData CreateDefaultTexture(TextureTypeFlagBits textureFlagBits);

        void SaveCache(const std::string& filePath, const ShaderTimeCache& shaderTimeCache) const;
        ShaderTimeCache LoadShaderCache(const std::string& filePath) const;

        void CompileShader(const std::string& filepath) const;

        static TextureFullData LoadDDSTexture(const std::string &filePath, TextureTypeFlagBits type);
        static TextureFullData LoadSTBTexture(const std::string &filePath, TextureTypeFlagBits type);
    public:
        //导入caiShader，输入为：caishader的路径，输出一个VkShaderModule，并且记录的修改时间caishader，实现懒加载
        ShaderModulePackages GetShaderCaIShaderModule(VkDevice device, const std::string& filePath,ShaderInfo& shaderInfo) const;
        ShaderInfo GetShaderInfo(VkDevice device, const std::string& filePath) const;
        ShaderModulePackages GetCompShaderModule(VkDevice device, const std::string& filePath, CompShaderInfo& compShaderInfo) const;
        std::vector<MeshData> LoadMesh(const std::string& fileName, ModelType modelType, TextureTypeFlags textureFlags, float scale = 1.0f);
        std::unique_ptr<ModelData> LoadModelData(const std::string& filePath, TextureTypeFlags textureFlags);
        std::vector<TextureFullData> LoadTextureFullDatas(aiMaterial* mat, const aiScene* scene,aiTextureType type, std::string directory);
        ExpectWithStr<std::unique_ptr<PrefabStruct>> LoadPrefabStruct(const std::string& filePath) const;
        TextureFullData LoadTextureFullData(const std::string& filePath, TextureTypeFlagBits type);
        void ReleaseTextureFullData(const TextureFullData& textureFullData);

        //实现异步
        //Async Func
        std::future<ShaderModulePackages> AsyncGetShaderCaIShaderModule(VkDevice device, const std::string& filePath) const;
        std::future<ShaderInfo> AsyncGetShaderInfo(VkDevice device, const std::string& filePath) const;
        std::future<ExpectWithStr<std::unique_ptr<PrefabStruct>>> AsyncLoadPrefabStruct(const std::string& filePath) const;

        static Resource& GetInstance();

        //路径暴露出来方便更改和调试
        std::string resourcePath{"../resources/"};
        std::string generalShaderPath{"../resources/shaders/glsl/"};
        std::string generalModelPath{"../resources/models/"};
        std::string caiShaderTimeCachePath{"../resources/CaIShaders/caiShaderTimeCache.bin"};
    };
}

#endif //RESOURCE_H
