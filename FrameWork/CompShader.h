//
// Created by cai on 2025/9/23.
//

#ifndef CAIENGINE_COMPSHADER_H
#define CAIENGINE_COMPSHADER_H
#include "vulkanFrameWork.h"

namespace FrameWork {
    class CompShader {
    public:
        static CompShader* Create(uint32_t& shaderID, const std::string& shaderPath);
        static void Destroy(const uint32_t &id);
        static void DestroyAll();
        static CompShader* Get(uint32_t id);
        static bool exist(uint32_t id);

        CompShader() = default;
        CompShader(const CompShader&) = delete;
        CompShader& operator=(const CompShader&) = delete;
        CompShader(CompShader&&) = default;
        CompShader& operator=(CompShader&&) = default;
        ~CompShader();


        CompShaderInfo GetShaderInfo() const;
        uint32_t GetPipelineID() const;

        void* GetShaderPropertyAddress(uint32_t materialDataID, const std::string& name, uint32_t id = 0);

        void Bind(const VkCommandBuffer& cmdBuffer) const;


    private:
        CompShader(const std::string& shaderPath); //计算着色器只需要使用shaderPath
        std::string shaderPath{};
        uint32_t pipelineID{UINT32_MAX};
        CompShaderInfo compShaderInfo{};


        inline static std::vector<CompShader*> compShaderPool{};
    };
}


#endif //CAIENGINE_COMPSHADER_H