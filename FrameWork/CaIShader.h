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
        static CaIShader* Create(uint32_t& id, const std::string& shaderPath, RenderPassType renderPassType);
        static CaIShader* Create(uint32_t& id, const std::string& shaderPath, VkFormat colorFormat = VK_FORMAT_UNDEFINED);
        static void Destroy(uint32_t& id);
        static CaIShader* Get(uint32_t id);
        static void DestroyAll();
        static bool exist(uint32_t id);


        ~CaIShader();

        CaIShader() = default;
        CaIShader(const CaIShader&) = delete;
        CaIShader& operator=(const CaIShader&) = delete;
        CaIShader(CaIShader&&) = default;
        CaIShader& operator=(CaIShader&&) = default;

        void* GetShaderPropertyAddress(uint32_t materialDataID, const std::string& name, uint32_t id = 0);//得到对应的地址方便映射

        void Bind(const VkCommandBuffer& cmdBuffer) const;
        //获取Shader
        ShaderInfo GetShaderInfo() const;
        uint32_t GetPipelineID() const;
    private:
        CaIShader(const std::string& shaderPath, RenderPassType renderPassType);
        CaIShader(const std::string& shaderPath, VkRenderPass renderPass);
        CaIShader(const std::string& shaderPath, VkFormat colorFormat);
        std::string shaderPath{};
        ShaderInfo shaderInfo{};
        uint32_t pipelineID{UINT32_MAX};

        inline static std::vector<CaIShader*> caiShaderPool{};
    };
}


#endif //CAIENGINE_CAISHADER_H