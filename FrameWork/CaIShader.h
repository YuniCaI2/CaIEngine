//
// Created by cai on 2025/9/4.
//

#ifndef CAIENGINE_CAISHADER_H
#define CAIENGINE_CAISHADER_H
#include<iostream>

#include "FrameWorkGUI.h"

namespace FrameWork {
    class CaIShader {
    public:
        CaIShader(const std::string& shaderPath, RenderPassType renderPassType);
        ~CaIShader();
    private:
        std::string shaderPath{};
        ShaderInfo shaderInfo{};
        uint32_t pipelineID{};
    };
}


#endif //CAIENGINE_CAISHADER_H