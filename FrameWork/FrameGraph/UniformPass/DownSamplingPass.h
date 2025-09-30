//
// Created by 51092 on 2025/9/29.
//

#ifndef CAIENGINE_DOWNSAMPLINGPASS_H
#define CAIENGINE_DOWNSAMPLINGPASS_H
#include "UniformPass.h"

namespace FG  {
    class DownSamplingPass : public UniformPass{
    public:
        virtual ~DownSamplingPass();
        DownSamplingPass(FrameGraph* frameGraph,uint32_t mipmapLevels);
        virtual void Bind() override;
        virtual void SetCreateResource(uint32_t& index) override;
        virtual void SetReadResource(const uint32_t& index) override;
        virtual void SetInputOutputResource(const uint32_t& index0, uint32_t& index1) override;

    private:
        static constexpr std::string_view shaderPath = "../resources/CaIShaders/Bloom/downSample.compshader";
        uint32_t mipmapLevels{};
        std::vector<uint32_t> compMaterials;
        uint32_t generateMipPasses;
        uint32_t generateAttachment; //这玩意是是输出
        uint32_t compShaderID{};

        FrameGraph* frameGraph{};
        uint32_t colorAttachment{};

    };
}


#endif //CAIENGINE_DOWNSAMPLINGPASS_H