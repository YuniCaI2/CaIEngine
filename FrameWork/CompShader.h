//
// Created by cai on 2025/9/23.
//

#ifndef CAIENGINE_COMPSHADER_H
#define CAIENGINE_COMPSHADER_H
#include "FrameWorkGUI.h"

namespace FrameWork {
    class CompShader {
    public:
        static uint32_t Create(const std::string& shaderPath);
        static void Destroy(uint32_t &id);
        static CompShader* Get(uint32_t id);

        CompShader() = default;
        CompShader(const CompShader&) = delete;
        CompShader& operator=(const CompShader&) = delete;
        CompShader(CompShader&&) = default;
        CompShader& operator=(CompShader&&) = default;

        CompShaderInfo GetShaderInfo() const;
        uint32_t GetPipelineID() const;


    private:
        CompShader(const std::string& shaderPath); //计算着色器只需要使用shaderPath
        std::string shaderPath{};
        uint32_t pipelineID{};
        CompShaderInfo cmdInfo{};


        inline static std::vector<CompShader*> compShaderPool{};
    };
}


#endif //CAIENGINE_COMPSHADER_H