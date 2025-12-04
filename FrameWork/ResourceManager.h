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
#include <map>
#include <expected>

#include "Logger.h"
#include "FrameGraph/ThreadPool.h"

namespace FrameWork {
    //这个类的作用是将外部的资源加载或者转为程序可用的资源
    using ShaderModulePackages = std::vector<std::pair<VkShaderStageFlagBits, VkShaderModule>>;
    class ResourceManager {
    private:
        //Task Queue
        class TaskQueue {
        public:
            TaskQueue() {
                taskThread = std::thread([this]() {
                    while (true) {
                        std::function<void()> task;
                        {
                            std::unique_lock<std::mutex> lock(this->taskQueueMutex);

                            this->cv.wait(lock, [this]{ return this->stop.load() || !this->taskQueue.empty(); });

                            if(this->stop.load() && this->taskQueue.empty()){
                                return;
                            }
                            task = std::move(this->taskQueue.front());
                            this->taskQueue.pop();
                        }
                        task();
                    }
                });
            };

            void Stop() {
                stop.store(true);
                cv.notify_all();
                if (taskThread.joinable()) {
                    taskThread.join();
                }
            }

            template<typename F, typename... Args>
            auto Enqueue(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
                using ResultType = std::invoke_result_t<F, Args...>;
                auto task = std::make_shared<std::packaged_task<ResultType()>>(
                    std::bind(std::forward<F>(f), std::forward<Args>(args)...)
                );
                std::future<ResultType> res = task->get_future();
                {
                    std::unique_lock<std::mutex> lock(taskQueueMutex);
                    if(stop.load()){
                        throw std::runtime_error("enqueue on stopped ThreadPool");
                    }
                    taskQueue.emplace([task](){ (*task)(); });
                }
                cv.notify_one();
                return res;
            }
        private:
            std::mutex taskQueueMutex;
            std::atomic<bool> stop{false};
            std::queue<std::function<void()>> taskQueue;
            std::condition_variable cv;
            std::thread taskThread;
        };
        TaskQueue taskQueue;

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
        TextureFullData CreateDefaultTexture(TextureTypeFlagBits textureFlagBits);

        void SaveCache(const std::string& filePath, const ShaderTimeCache& shaderTimeCache) const;
        ShaderTimeCache LoadShaderCache(const std::string& filePath) const;

        void CompileShader(const std::string& filepath) const;

        static TextureFullData LoadDDSTexture(const std::string &filePath, TextureTypeFlagBits type);
        static TextureFullData LoadSTBTexture(const std::string &filePath, TextureTypeFlagBits type);


        static constexpr std::pair<aiTextureType, const char*>  aiTextureTypeToString[] = {
            { aiTextureType_DIFFUSE,           "albedo" },
            { aiTextureType_NORMALS,           "normal" },
            { aiTextureType_SPECULAR,          "specular" },
            { aiTextureType_METALNESS,         "metallic" },
            { aiTextureType_DIFFUSE_ROUGHNESS, "roughness" },
            { aiTextureType_AMBIENT_OCCLUSION, "ao" },
            { aiTextureType_EMISSIVE,          "emissive" },
            { aiTextureType_OPACITY,           "opacity" }
        };
    public:
        //导入caiShader，输入为：caishader的路径，输出一个VkShaderModule，并且记录的修改时间caishader，实现懒加载
        ShaderModulePackages GetShaderCaIShaderModule(VkDevice device, const std::string& filePath,ShaderInfo& shaderInfo) const;
        ShaderInfo GetShaderInfo(const std::string& filePath) const;
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

        //Default Source Path
        std::string defaultMaterialPath = "../DefaultAsset/DefaultMaterial"; //加载模型时使用, 组织结构
        std::string defaultShaderPath = "../../DefaultAsset/DefaultShader.json";
        std::string defaultShaderPassPath = "../../DefaultAsset/DefaultShader.caishader";
    };
}

#endif //RESOURCE_H
