//
// Created by cai on 2025/9/4.
//

#ifndef CAIENGINE_CAIMATERIAL_H
#define CAIENGINE_CAIMATERIAL_H
#include<string>
#include "CaIShader.h"
#include "Logger.h"
#include "PublicStruct.h"
namespace FrameWork {
    class CaIMaterial {
    public:
        static CaIMaterial* Create(uint32_t& id, uint32_t shaderRef);
        static void Destroy(uint32_t& id);
        static CaIMaterial* Get(uint32_t id);
        static void DestroyAll();
        static bool exist(uint32_t id);

        ~CaIMaterial();

        //因为其构造的原因我这边不允许移动
        static Handle<CaIMaterial> CreateHandle(const Handle<CaIShader>& shaderHandle);
        static bool Bind(Handle<CaIMaterial>& handle, const VkCommandBuffer& cmdBuffer);
        static uint32_t& GetMaterialDataID(const Handle<CaIMaterial>& handle);
        static Handle<CaIShader> GetShaderHandle(const Handle<CaIMaterial>& handle);



        template<typename Param>
        CaIMaterial& SetParam(const std::string& name, const Param& param, uint32_t index = 0) {
            if (CaIShader::Get(shaderRef) == nullptr) {
                LOG_ERROR("Can't set the param {} ,the material Shader has been destroyed", name);
                return *this;
            }
            auto address = CaIShader::Get(shaderRef)->GetShaderPropertyAddress(dataID, name, index);
            if (address != nullptr) {
                memcpy(address, &param, sizeof(Param));
            }
            return *this;
        }

        template<typename Param>
        static void SetParam(const Handle<CaIMaterial>& handle, const std::string& name, const Param& param, uint32_t index = 0) {
            FrameWork::CaIMaterial* material = caiMaterialWrappedPool.GetResource(handle.index);
            if (CaIShader::Get(material->shaderHandle) == nullptr) {
                LOG_ERROR("Can't set the param {} ,the material Shader has been destroyed", name);
            }
            auto address = CaIShader::Get(material->shaderHandle)->GetShaderPropertyAddress(material->dataID, name, index);
            if (address != nullptr) {
                memcpy(address, &param, sizeof(Param));
            }
        }

        CaIMaterial &SetTexture(const std::string &name, uint32_t id) ;
        CaIMaterial& SetAttachment(const std::string& name, uint32_t id) ;
        //因为有时候会将Attachment作为纹理输入比如呈现或者后处理，飞行帧资源上不同

        static void SetTexture(const Handle<CaIMaterial>& handle, const std::string& name, uint32_t id);
        static void SetAttachment(const Handle<CaIMaterial>& handle, const std::string& name, uint32_t id);


        [[nodiscard]] CaIShader* GetShader() const;
        [[nodiscard]] uint32_t GetShaderID() const;

        void Bind(const VkCommandBuffer& cmdBuffer) const;

        uint32_t dataID {UINT32_MAX}; //对应VulkanMaterialData
    private:
        template<typename T, typename Enable>
        friend struct ResourceHandleTraits;
        template<typename T>
        friend struct ResourcePool;

        CaIMaterial() = default;
        explicit CaIMaterial(uint32_t shaderRef); //保证不发生隐式转换
        explicit CaIMaterial(const Handle<CaIShader>& shaderHandle);
        CaIMaterial(const CaIMaterial&) = delete;
        CaIMaterial& operator=(const CaIMaterial&) = delete;
        CaIMaterial(CaIMaterial&&) = default;
        CaIMaterial& operator=(CaIMaterial&&) = default;
        inline static std::vector<CaIMaterial*> caiMaterialPools{};
        uint32_t shaderRef {UINT32_MAX};
        Handle<CaIShader> shaderHandle{};
        inline static ResourcePool<CaIMaterial> caiMaterialWrappedPool;
    };

    template<typename T> //后面这个槽位用来实现SFINAE
    struct ResourceHandleTraits <T, std::enable_if_t<std::is_same_v<CaIMaterial, T>>>{
        static void Destroy(uint32_t index) {
            CaIMaterial::caiMaterialWrappedPool.Delete(index, [](T* ptr){
                delete ptr;
            });
        }
        static uint32_t Copy(uint32_t index) {
            return CaIMaterial::caiMaterialWrappedPool.Copy(index);
        }

        static bool exist(uint32_t index) {
            return CaIMaterial::caiMaterialWrappedPool.Exist(index);
        }

    };
}

#endif //CAIENGINE_CAIMATERIAL_H
