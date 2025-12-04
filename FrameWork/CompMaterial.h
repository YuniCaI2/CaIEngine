//
// Created by cai on 2025/9/23.
//

#ifndef CAIENGINE_COMPMATERIAL_H
#define CAIENGINE_COMPMATERIAL_H
#include <cstdint>
#include <string>

#include "CompShader.h"
#include "Logger.h"
#include "PublicStruct.h"


namespace FrameWork {
    class CompMaterial {
    public:
        static Handle<CompMaterial> CreateHandle(const Handle<CompShader>& shaderHandle);
        static bool Bind(Handle<CompMaterial>& handle, const VkCommandBuffer& cmdBuffer);
        static uint32_t& GetMaterialDataID(const Handle<CompMaterial>& handle);
        static Handle<CompShader> GetShaderHandle(const Handle<CompMaterial>& handle);

        CompMaterial(CompMaterial&&) = default;
        CompMaterial& operator=(CompMaterial&&) = default;
        CompMaterial(const CompMaterial&) = delete;
        CompMaterial& operator=(CompMaterial&) = delete;
        ~CompMaterial();

        template<typename Param>
        static void SetParam(const Handle<CompMaterial>& handle, const std::string& name, const Param& param, uint32_t index = 0) {
            FrameWork::CompMaterial* material = compMaterialWrappedPool.GetResource(handle.index);
            if (!material->shaderHandle) {
                LOG_ERROR("Can't set the param {} ,the material Shader has been destroyed", name);
            }
            auto address = CompShader::GetShaderPropertyAddress(material->shaderHandle, handle, name, index);
            if (address != nullptr) {
                memcpy(address, &param, sizeof(Param));
            }
        }

        static void SetTexture(const Handle<CompMaterial>& handle, const std::string& name, uint32_t id);
        static void SetAttachment(const Handle<CompMaterial>& handle, const std::string& name, uint32_t id);
        static void SetStorageImage2D(const Handle<CompMaterial>& handle, const std::string& name, uint32_t id, uint32_t baseMipmap = 0, bool isStatic = false);
        static void SetStorageBuffer(const Handle<CompMaterial>& handle, const std::string& name, uint32_t id, bool isStatic = false);

        void Bind(const VkCommandBuffer& commandBuffer) const;

        uint32_t compDataID{UINT32_MAX};
    private:
        template<typename T, typename Enable>
        friend struct ResourceHandleTraits;
        template<typename T>
        friend struct ResourcePool;

        CompMaterial() = default;
        explicit CompMaterial(const Handle<CompShader>& shaderHandle);
        inline static ResourcePool<CompMaterial> compMaterialWrappedPool{};
        //Handle
        Handle<CompShader> shaderHandle{};
    };
    template<typename T> //后面这个槽位用来实现SFINAE
    struct ResourceHandleTraits <T, std::enable_if_t<std::is_same_v<CompMaterial, T>>>{
        static void Destroy(uint32_t index) {
            CompMaterial::compMaterialWrappedPool.Delete(index, [](T* ptr){
                delete ptr;
            });
        }
        static uint32_t Copy(uint32_t index) {
            return CompMaterial::compMaterialWrappedPool.Copy(index);
        }

        static bool exist(uint32_t index) {
            return CompMaterial::compMaterialWrappedPool.Exist(index);
        }

    };
}


#endif //CAIENGINE_COMPMATERIAL_H
