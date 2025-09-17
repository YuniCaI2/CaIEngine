//
// Created by cai on 2025/9/4.
//

#ifndef CAIENGINE_CAIMATERIAL_H
#define CAIENGINE_CAIMATERIAL_H
#include<string>
#include<queue>
#include "CaIShader.h"
#include "Logger.h"
namespace FrameWork {
    class CaIMaterial {
    public:
        static CaIMaterial* Create(uint32_t& id, uint32_t shaderRef);
        static void Destroy(uint32_t& id);
        static CaIMaterial* Get(uint32_t id);
        static void DestroyAll();
        static bool exist(uint32_t id);
        static void PendingSetAttachments(); //帧头执行，意思是等待前面的实现完成

        ~CaIMaterial();

        //因为其构造的原因我这边不允许移动



        template<typename Param>
        void SetParam(const std::string& name, const Param& param, uint32_t index) {
            if (CaIShader::Get(shaderRef) == nullptr) {
                LOG_ERROR("Can't set the param {} ,the material Shader has been destroyed", name);
                return;
            }
            auto address = CaIShader::Get(shaderRef)->GetShaderPropertyAddress(dataID, name, index);
            if (address != nullptr) {
                memcpy(address, &param, sizeof(Param));
            }
        }

        void SetTexture(const std::string& name, uint32_t id) const;
        void SetAttachment(const std::string& name, uint32_t id) ; //因为有时候会将Attachment作为纹理输入比如呈现或者后处理，飞行帧资源上不同

        [[nodiscard]] CaIShader* GetShader() const;

        void Bind(const VkCommandBuffer& cmdBuffer) const;

        uint32_t dataID {UINT32_MAX}; //对应VulkanMaterialData
    private:
        CaIMaterial() = default;
        explicit CaIMaterial(uint32_t shaderRef); //保证不发生隐式转换
        CaIMaterial(const CaIMaterial&) = delete;
        CaIMaterial& operator=(const CaIMaterial&) = delete;
        CaIMaterial(CaIMaterial&&) = default;
        CaIMaterial& operator=(CaIMaterial&&) = default;
        void PendingSetAttachment_(); //在帧头部之执行
        inline static std::vector<CaIMaterial*> caiMaterialPools{};
        uint32_t shaderRef {UINT32_MAX};
        std::queue<std::pair<std::string, uint32_t>> pendingAttachments{};
    };
}

#endif //CAIENGINE_CAIMATERIAL_H