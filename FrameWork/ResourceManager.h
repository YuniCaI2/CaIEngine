//
// Created by 51092 on 25-6-13.
//

#ifndef RESOURCE_H
#define RESOURCE_H
#include <mutex>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <future>
#include <assimp/scene.h>
#include <shared_mutex>
#include "PublicEnum.h"
#include "PublicStruct.h"
#include "Asset/AssetHead.h"
#include <map>
#include <expected>

#include "Logger.h"

namespace FrameWork {
    //这个类的作用是将外部的资源加载或者转为程序可用的资源
    using ShaderModulePackages = std::vector<std::pair<VkShaderStageFlagBits, VkShaderModule>>;
    class ResourceManager {
    private:
        ResourceManager();
        ~ResourceManager();
        std::unordered_map<std::string, TextureFullData> textureMap;
        using ShaderTimeCache =  std::map<std::string, std::filesystem::file_time_type>;
        mutable ShaderTimeCache shaderTimeCache;
        //记录自己的shader改变情况
        mutable ShaderTimeCache caiShaderTimeCache;
        mutable std::mutex caiShaderTimeCacheMutex;

        void processNode(aiNode *node, const aiScene *scene, std::vector<MeshData>& meshes, ModelType modelType, std::string, TextureTypeFlags textureFlags);
        FrameWork::MeshData processMesh(aiMesh *mesh, ModelType modelType, const aiScene *scene, std::string, TextureTypeFlags textureFlags);
        std::unique_ptr<ModelNode> LoadModelNode_Impl(std::unique_ptr<ModelNode> modelNode , aiScene* scene, aiNode* node,const std::string& directory,ModelType modelType, TextureTypeFlags textureFlags);
        TextureFullData CreateDefaultTexture(TextureTypeFlagBits textureFlagBits);

        void SaveCache(const std::string& filePath, const ShaderTimeCache& shaderTimeCache) const;
        ShaderTimeCache LoadShaderCache(const std::string& filePath) const;

        void CompileShader(const std::string& filepath) const;

        static TextureFullData LoadDDSTexture(const std::string &filePath, TextureTypeFlagBits type);
        static TextureFullData LoadSTBTexture(const std::string &filePath, TextureTypeFlagBits type);
        static void LoadDDSTextureAsset(const std::string& filePath, TextureAsset& textureAsset);
        static void LoadSTBTextureAsset(const std::string& filePath, TextureAsset& textureAsset);

        //Asset Pool
        std::shared_mutex textureAssetPoolMutex;
        std::vector<std::shared_ptr<TextureAsset>> textureAssetPool;
        std::shared_mutex materialAssetPoolMutex;
        std::vector<std::shared_ptr<MaterialAsset>> materialAssetPool;
        std::shared_mutex shaderAssetPoolMutex;
        std::vector<std::shared_ptr<ShaderAsset>> shaderAssetPool;
        std::shared_mutex meshAssetPoolMutex;
        std::vector<std::shared_ptr<MeshAsset>> meshAssetPool;
        std::shared_mutex modelAssetPoolMutex;
        std::vector<std::shared_ptr<ModelAsset>> modelAssetPool;

        template<typename T>
        std::shared_mutex& GetAssetPoolMutex() {
            if constexpr (std::is_same_v<T, MeshAsset>) {
                return meshAssetPoolMutex;
            }
            if constexpr (std::is_same_v<T, MaterialAsset>) {
                return materialAssetPoolMutex;
            }
            if constexpr (std::is_same_v<T, ModelAsset>) {
                return modelAssetPoolMutex;
            }
            if constexpr (std::is_same_v<T, TextureAsset>) {
                return textureAssetPoolMutex;
            }
            if constexpr (std::is_same_v<T, ShaderAsset>) {
                return shaderAssetPoolMutex;
            }
        }

        template<typename T>
        std::vector<std::shared_ptr<T>>& GetAssetPool() {
            if constexpr (std::is_same_v<T, MeshAsset>) {
                return meshAssetPool;
            }
            if constexpr (std::is_same_v<T, MaterialAsset>) {
                return materialAssetPool;
            }
            if constexpr (std::is_same_v<T, ModelAsset>) {
                return modelAssetPool;
            }
            if constexpr (std::is_same_v<T, TextureAsset>) {
                return textureAssetPool;
            }
            if constexpr (std::is_same_v<T, ShaderAsset>) {
                return shaderAssetPool;
            }
        }

        template<class T>
        uint32_t AddAsset(const std::shared_ptr<T>& asset) {
            auto& mutex = GetAssetPoolMutex<T>();
            std::scoped_lock lock(mutex);
            auto& assetPool = GetAssetPool<T>();
            for (int i = 0; i < assetPool.size(); i++) {
                //同名替换
                if (assetPool[i]->name == asset->name) {
                    assetPool[i] = asset;
                    return i;
                }
            }
            for (int i = 0; i < assetPool.size(); i++) {
                if (assetPool[i] == nullptr) {
                    assetPool[i] = asset;
                    return i;
                }
            }
            assetPool.push_back(std::move(asset));
            return assetPool.size() - 1;
        }

        template<class T>
        void DeleteAsset(uint32_t index) {
            auto& mutex = GetAssetPoolMutex<T>();
            std::scoped_lock lock(mutex);
            auto& assetPool = GetAssetPool<T>();
            if (index < assetPool.size()) {
                assetPool[index].reset();
            }else {
                LOG_ERROR("Delete asset failed Because out of range !");
                throw std::out_of_range("Delete asset failed Because out of range !");
            }
        }

        template<class T>
        std::shared_ptr<T> GetAsset(uint32_t index) {
            auto& mutex = GetAssetPoolMutex<T>();
            std::scoped_lock lock(mutex);
            auto& assetPool = GetAssetPool<T>();
            if (index < assetPool.size()) {
                return assetPool[index];
            }else {
                LOG_ERROR("Get asset failed Because out of range !");
                throw std::out_of_range("Delete asset failed Because out of range !");
            }
        }



        //Table
        //Path To Index
        std::unordered_map<std::string, uint32_t> texturePathToIndex;
        std::unordered_map<std::string, uint32_t> materialPathToIndex;
        std::unordered_map<std::string, uint32_t> shaderPathToIndex;
        std::unordered_map<std::string, uint32_t> meshPathToIndex;
        std::unordered_map<std::string, uint32_t> modelPathToIndex;

        std::shared_mutex texturePathToIndexMutex;
        std::shared_mutex materialPathToIndexMutex;
        std::shared_mutex shaderPathToIndexMutex;
        std::shared_mutex meshPathToIndexMutex;
        std::shared_mutex modelPathToIndexMutex;



        //AssetCache
        std::string assetCacheTablePath = "../../AssetCache/assetTable.json";
        // std::string assetCacheTablePath = "../AssetCache/assetTable.json";
        std::unordered_map<std::string, std::string> assetCacheTable; //原始路径到cache路径映射
        std::string assetCachePath =  "../../AssetCache/";
        // std::string assetCachePath =  "../AssetCache/";

    public:
        //Asset Op
        // Load 对应JSON
        uint32_t LoadTextureAssetFromJSON(const std::string& path); //加载纹理且生成默认Import 和.bin
        uint32_t LoadMaterialAssetFromJSON(const std::string& path);
        uint32_t LoadShaderAssetFromJSON(const std::string& path);
        uint32_t LoadModelAssetFromJSON(const std::string& path);
        uint32_t LoadMeshAssetFromJSON(const std::string& path);

        uint32_t LoadTextureAssetFromSource(const std::string& path);
        uint32_t LoadModelAssetFromSource(const std::string& path);
        uint32_t LoadShaderAssetFromSource(const std::string& path);


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

        static ResourceManager& GetInstance();

        //路径暴露出来方便更改和调试
        std::string resourcePath{"../resources/"};
        std::string generalShaderPath{"../resources/shaders/glsl/"};
        std::string generalModelPath{"../resources/models/"};
        std::string caiShaderTimeCachePath{"../resources/CaIShaders/caiShaderTimeCache.bin"};

    };
}

#endif //RESOURCE_H
