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

#include "Schema.h"
#include "Logger.h"
#include "PublicEnum.h"
#include "PublicStruct.h"
#include <map>
#include <expected>
namespace FrameWork {
    //这个类的作用是将外部的资源加载或者转为程序可用的资源
    using ShaderModulePackages = std::vector<std::pair<VkShaderStageFlagBits, VkShaderModule>>;
    class ResourceManager {
    private:
        ResourceManager();



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

        std::unique_ptr<ModelNode> LoadModelNode(const std::string& filePath, TextureTypeFlags textureFlags);

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

        //Asset Pool
        enum class AssetErrorCode {
            GUID_NOT_FOUND,
            ASSET_NOT_FOUND,
        };

        template<typename Meta, typename Asset>
        requires std::derived_from<Meta, BaseMeta>
        struct MetaAsset {
            std::unique_ptr<Meta> meta;
            std::unique_ptr<Asset> asset;
        };

        template<typename Meta, typename Asset>
        requires std::derived_from<Meta, BaseMeta>
        struct MetaAssetPool {
            std::shared_mutex mutex;
            std::unordered_map<GUID, MetaAsset<Meta, Asset>> metaAssetPool;
            ExpectedWithInfo<Meta*> GetMeta(const GUID& guid) {
                std::shared_lock lock(mutex);
                auto it = metaAssetPool.find(guid);
                if (it == metaAssetPool.end() || it->second.meta == nullptr) {
                    return std::unexpected(ErrorInfo("Can't find GUID Meta"));
                }
                return ExpectedWithInfo<Meta*>(it->second.meta.get());
            }

            ExpectedWithInfo<Asset*> GetAsset(const GUID& guid) {
                std::shared_lock lock(mutex);
                auto it = metaAssetPool.find(guid);
                if (it == metaAssetPool.end() || it->second.asset == nullptr) {
                    return std::unexpected(ErrorInfo("Can't find GUID Asset"));
                }
                return ExpectedWithInfo<Asset*>(it->second.asset.get());
            }

            void SetMeta(const GUID& guid, std::unique_ptr<Meta> meta){
                std::lock_guard<std::mutex> lock(mutex);
                if (metaAssetPool.contains(guid)) {
                    LOG_WARNING("The GUID: {} has been existed, the : {} will be overwritten",
                        guid, metaAssetPool[guid].meta->name);
                }
                metaAssetPool[guid].meta = std::move(meta);
            }

            void SetAsset(const GUID& guid, const Asset& asset){
                std::lock_guard<std::mutex> lock(mutex);
                if (metaAssetPool.contains(guid)) {
                    LOG_WARNING("The GUID: {} has been existed, the : {} will be overwritten",
                        guid, metaAssetPool[guid].asset->name);
                }
                metaAssetPool[guid].asset = std::move(asset);
            }
        };

        MetaAssetPool<TextureMeta, TextureAsset> textureAssetPool;
        MetaAssetPool<MaterialMeta, MaterialAsset> materialAssetPool;
        MetaAssetPool<ShaderMeta, ShaderAsset> shaderAssetPool;
        MetaAssetPool<ModelMeta, ModelAsset> modelAssetPool;
    };
}

#endif //RESOURCE_H
