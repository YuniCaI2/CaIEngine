//
// Created by 51092 on 2025/9/30.
//

#ifndef CAIENGINE_BLOOMINGPASS_H
#define CAIENGINE_BLOOMINGPASS_H
#include "UniformPass.h"
#include "DownSamplingPass.h"

namespace FG {
    class BloomingPass : public UniformPass{
    public:
        virtual ~BloomingPass();
        BloomingPass(FrameGraph* frameGraph, uint32_t mipmapLevels); //降采样层数
        virtual void Bind() override;
        virtual void SetCreateResource(uint32_t &index) override;
        virtual void SetReadResource(const uint32_t &index) override;
        virtual void SetInputOutputResource(const uint32_t &index0, uint32_t &index1) override;
    private:
        static constexpr std::string_view rowShaderPath =  "../resources/CaIShaders/Bloom/upRowSample.compshader";
        static constexpr std::string_view colShaderPath =  "../resources/CaIShaders/Bloom/upColSample.compshader";
        static constexpr std::string_view specShaderPath = "../resources/CaIShaders/Bloom/getSpec.compshader";
        static constexpr std::string_view blendShaderPath = "../resources/CaIShaders/Bloom/blend.compshader";

        uint32_t mipmapLevels;

        std::vector<uint32_t> compRowMaterials;
        std::vector<uint32_t> compColMaterials;
        uint32_t getSpecMaterialID;
        uint32_t blendMaterialID;

        uint32_t generatePass;
        uint32_t specPass;
        uint32_t generateAttachment;
        uint32_t rowShaderID;
        uint32_t colShaderID;
        uint32_t specShaderID;
        uint32_t blendShaderID;

        FrameGraph* frameGraph;
        uint32_t colorAttachment;//附件不需要MipmapLevel
        uint32_t specAttachment;
        uint32_t downSamplingAttachment;

        DownSamplingPass downSampling;
    };
}


#endif //CAIENGINE_BLOOMINGPASS_H