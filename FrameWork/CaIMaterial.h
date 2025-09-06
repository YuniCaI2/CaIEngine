//
// Created by cai on 2025/9/4.
//

#ifndef CAIENGINE_CAIMATERIAL_H
#define CAIENGINE_CAIMATERIAL_H
#include<string>

#include "CaIShader.h"
#include "Logger.h"

namespace FrameWork {
    class CaIMaterial {
    public:
        explicit CaIMaterial(std::weak_ptr<CaIShader> shader); //保证不发生隐式转换
        ~CaIMaterial();

        //因为其构造的原因我这边不允许移动
        CaIMaterial() = default;
        CaIMaterial(const CaIMaterial&) = delete;
        CaIMaterial& operator=(const CaIMaterial&) = delete;
        CaIMaterial(CaIMaterial&&) = default;
        CaIMaterial& operator=(CaIMaterial&&) = default;


        template<typename Param>
        void SetParam(const std::string& name, const Param& param, uint32_t index) {
            if (shaderRef.expired()) {
                LOG_ERROR("Can't set the param {} ,the material Shader has been destroyed", name);
                return;
            }
            auto address = shaderRef.lock()->GetShaderPropertyAddress(dataID, name, index);
            if (address != nullptr) {
                memcpy(address, &param, sizeof(Param));
            }
        }

        void SetTexture(const std::string& name, uint32_t id) const;
        void SetAttachment(const std::string& name, uint32_t id) const; //因为有时候会将Attachment作为纹理输入比如呈现或者后处理，飞行帧资源上不同

        [[nodiscard]] std::weak_ptr<CaIShader> GetShader() const;

        void Bind(const VkCommandBuffer& cmdBuffer) const;

        uint32_t dataID {UINT32_MAX}; //对应VulkanMaterialData
    private:
        std::weak_ptr<CaIShader> shaderRef;
    };
}

#endif //CAIENGINE_CAIMATERIAL_H