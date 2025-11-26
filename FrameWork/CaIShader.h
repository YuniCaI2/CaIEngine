//
// Created by cai on 2025/9/4.
//

#ifndef CAIENGINE_CAISHADER_H
#define CAIENGINE_CAISHADER_H
#include "PublicStruct.h"
#include <mutex>

namespace FrameWork {
    class CaIShader {
    public:
        //类似Vulkan中的资源
        static CaIShader* Create(uint32_t& id, const std::string& shaderPath, VkFormat colorFormat = VK_FORMAT_UNDEFINED);
        static void Destroy(uint32_t& id);
        static CaIShader* Get(uint32_t id);
        static void DestroyAll();
        static bool exist(uint32_t id);

        static Handle<CaIShader> Create(const std::string& shaderPath, VkFormat colorFormat = VK_FORMAT_UNDEFINED);
        static void Destroy(Handle<CaIShader>& handle);
        static CaIShader* Get(Handle<CaIShader>& handle);
        static bool exist(Handle<CaIShader>& handle);
        static Handle<CaIShader> Copy(const Handle<CaIShader>& handle);



        ~CaIShader();

        CaIShader() = default;
        CaIShader(const CaIShader&) = delete;
        CaIShader& operator=(const CaIShader&) = delete;
        CaIShader(CaIShader&&) = default;
        CaIShader& operator=(CaIShader&&) = default;

        void* GetShaderPropertyAddress(uint32_t materialDataID, const std::string& name, uint32_t id = 0);//得到对应的地址方便映射

        void Bind(const VkCommandBuffer& cmdBuffer) const;
        //获取Shader
        ShaderInfo GetShaderInfo() const;
        uint32_t GetPipelineID() const;
    private:
        CaIShader(const std::string& shaderPath, VkFormat colorFormat);
        std::string shaderPath{};
        ShaderInfo shaderInfo{};
        uint32_t pipelineID{UINT32_MAX};

        inline static std::vector<CaIShader*> caiShaderPool{};
        inline static std::mutex caiShaderWrappedPoolMutex{};
        inline static std::vector<ResourceWrapper<CaIShader>> caiShaderWrappedPool{};
    };
    template <>
    struct Handle<CaIShader> {
        uint32_t index{UINT32_MAX};

        Handle() = default;
        Handle(const Handle& handle) {
        }
        Handle &operator=(const Handle &handle) = default;
        Handle(Handle &&handle) noexcept = default;
        Handle &operator=(Handle &&handle) noexcept = default; //Handle拷贝不在这
    };
}



#endif //CAIENGINE_CAISHADER_H