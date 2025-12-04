//
// Created by cai on 2025/9/4.
//

#ifndef CAIENGINE_CAISHADER_H
#define CAIENGINE_CAISHADER_H
#include "PublicStruct.h"
#include <mutex>

namespace FrameWork {
    class CaIMaterial;
    class CaIShader {
    public:
        //类似Vulkan中的资源
        static Handle<CaIShader> CreateHandle(const std::string& shaderPath, VkFormat colorFormat = VK_FORMAT_UNDEFINED);
        static bool Bind(Handle<CaIShader>& handle, const VkCommandBuffer& cmdBuffer);
        static ShaderInfo GetInfo(const Handle<CaIShader>& shaderHandle);
        static void* GetShaderPropertyAddress(const Handle<CaIShader>& shaderHandle, const Handle<CaIMaterial>& materialHandle, const std::string& name, uint32_t id = 0);
        static uint32_t GetPipelineID(const Handle<CaIShader>& pipelineShader);



        ~CaIShader();

        CaIShader() = default;
        CaIShader(const CaIShader&) = delete;
        CaIShader& operator=(const CaIShader&) = delete;
        CaIShader(CaIShader&&) = default;
        CaIShader& operator=(CaIShader&&) = default;

        void Bind(const VkCommandBuffer& cmdBuffer) const;
        //获取Shader
        ShaderInfo GetShaderInfo() const;
        uint32_t GetPipelineID() const;
    private:


        template<typename T, typename Enable>
        friend struct ResourceHandleTraits;
        template<typename T>
        friend struct ResourcePool;
        CaIShader(const std::string& shaderPath, VkFormat colorFormat);
        std::string shaderPath{};
        ShaderInfo shaderInfo{};
        uint32_t pipelineID{UINT32_MAX};

        inline static std::mutex caiShaderWrappedPoolMutex{};
        inline static ResourcePool<CaIShader> caiShaderWrappedPool;
    };

    template<typename T> //后面这个槽位用来实现SFINAE
    struct ResourceHandleTraits <T, std::enable_if_t<std::is_same_v<CaIShader, T>>>{
        static void Destroy(uint32_t index) {
            CaIShader::caiShaderWrappedPool.Delete(index, [](T* ptr){
                delete ptr;
            });
        }
        static uint32_t Copy(uint32_t index) {
            return CaIShader::caiShaderWrappedPool.Copy(index);
        }

        static bool exist(uint32_t index) {
            return CaIShader::caiShaderWrappedPool.Exist(index);
        }
    };
}



#endif //CAIENGINE_CAISHADER_H
