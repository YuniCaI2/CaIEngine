//
// Created by cai on 2025/9/4.
//

#ifndef CAIENGINE_CAISHADER_H
#define CAIENGINE_CAISHADER_H
#include<iostream>
#include "PublicStruct.h"

namespace FrameWork {
    class CaIShader {
    public:
        CaIShader(const std::string& shaderPath, RenderPassType renderPassType);
        ~CaIShader();

        void* GetShaderPropertyAddress(uint32_t materialDataID, const std::string& name, uint32_t id = 0);//得到对应的地址方便映射

        void Bind(const VkCommandBuffer& cmdBuffer) const;
        //获取Shader
        ShaderInfo GetShaderInfo() const;
        uint32_t GetPipelineID() const;
    private:
        std::string shaderPath{};
        ShaderInfo shaderInfo{};
        uint32_t pipelineID{};
    };
}


#endif //CAIENGINE_CAISHADER_H