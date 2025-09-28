//
// Created by cai on 2025/9/23.
//

#ifndef CAIENGINE_COMPMATERIAL_H
#define CAIENGINE_COMPMATERIAL_H
#include <cstdint>
#include <string>
#include <vector>

#include "CompShader.h"
#include "Logger.h"


namespace FrameWork {
    class CompMaterial {
    public:
        static CompMaterial* Create(uint32_t& materialID, uint32_t& shaderID);
        static void Destroy(const uint32_t& id);
        static CompMaterial* Get(uint32_t& id);
        static void DestroyAll();
        static bool exist(uint32_t id);

        CompMaterial(CompMaterial&&) = default;
        CompMaterial& operator=(CompMaterial&&) = default;
        CompMaterial(const CompMaterial&) = delete;
        CompMaterial& operator=(CompMaterial&) = delete;
        ~CompMaterial();

        template<typename Param>
        CompMaterial& SetParam(const std::string& name, Param& param, uint32_t index = 0) {
            if (CompShader::Get(shaderRef) == nullptr) {
                LOG_ERROR("CompShader::SetMaterial: CompShader is nullptr , id : {}", shaderRef);
            }
            auto address = CompShader::Get(shaderRef)->GetShaderPropertyAddress(compDataID, name, index);
            if (address != nullptr) {
                memcpy(address, &param, sizeof(Param));
            }
            return *this;
        }
        CompMaterial& SetTexture(const std::string& name, uint32_t id);
        CompMaterial& SetAttachment(const std::string& name, uint32_t id);
        CompMaterial& SetStorageImage2D(const std::string& name, uint32_t id, uint32_t baseMipmap = 0, bool isStatic = false);
        CompMaterial& SetStorageBuffer(const std::string& name, uint32_t id, bool isStatic = false);

        CompShader* GetShader() const;

        void Bind(const VkCommandBuffer& commandBuffer) const;

        uint32_t compDataID{UINT32_MAX};
    private:
        CompMaterial() = default;
        explicit CompMaterial(uint32_t shaderRef);

        inline static std::vector<CompMaterial*> compMaterialPool{};
        uint32_t shaderRef{UINT32_MAX};
    };
}


#endif //CAIENGINE_COMPMATERIAL_H