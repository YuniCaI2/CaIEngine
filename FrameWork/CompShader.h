//
// Created by cai on 2025/9/23.
//

#ifndef CAIENGINE_COMPSHADER_H
#define CAIENGINE_COMPSHADER_H
#include "PublicStruct.h"

namespace FrameWork {
    class CompMaterial;
    class CompShader {
    public:
        //Handle
        static Handle<CompShader> CreateHandle(const std::string& shaderPath);
        static bool Bind(Handle<CompShader>& handle, const VkCommandBuffer& cmdBuffer);
        static CompShaderInfo GetInfo(const Handle<CompShader>& shaderHandle);
        static void* GetShaderPropertyAddress(const Handle<CompShader>& shaderHandle, const Handle<CompMaterial>& materialHandle, const std::string& name, uint32_t id = 0);
        static uint32_t GetPipelineID(const Handle<CompShader>& pipelineShader);

        CompShader() = default;
        CompShader(const CompShader&) = delete;
        CompShader& operator=(const CompShader&) = delete;
        CompShader(CompShader&&) = default;
        CompShader& operator=(CompShader&&) = default;
        ~CompShader();


        CompShaderInfo GetShaderInfo() const;
        uint32_t GetPipelineID() const;

        void Bind(const VkCommandBuffer& cmdBuffer) const;


    private:
        template<typename T, typename Enable>
        friend struct ResourceHandleTraits;
        template<typename T>
        friend struct ResourcePool;
        CompShader(const std::string& shaderPath); //计算着色器只需要使用shaderPath
        std::string shaderPath{};
        uint32_t pipelineID{UINT32_MAX};
        CompShaderInfo compShaderInfo{};


        inline static std::vector<CompShader*> compShaderPool{};
        inline static ResourcePool<CompShader> compShaderWrappedPool{};
    };

    template<typename T> //后面这个槽位用来实现SFINAE
    struct ResourceHandleTraits <T, std::enable_if_t<std::is_same_v<CompShader, T>>>{
        static void Destroy(uint32_t index) {
            CompShader::compShaderWrappedPool.Delete(index, [](T* ptr){
                delete ptr;
            });
        }
        static uint32_t Copy(uint32_t index) {
            return CompShader::compShaderWrappedPool.Copy(index);
        }

        static bool exist(uint32_t index) {
            return CompShader::compShaderWrappedPool.Exist(index);
        }
    };
}


#endif //CAIENGINE_COMPSHADER_H
