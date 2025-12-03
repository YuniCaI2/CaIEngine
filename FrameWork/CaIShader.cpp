//
// Created by cai on 2025/9/4.
//

#include "CaIShader.h"
#include "Logger.h"
#include "PublicStruct.h"
#include"vulkanFrameWork.h"
#include <cstdint>
#include <stdexcept>

//TODO: 添加错误处理保证后续热加载CaIShader时不影响原本的程序运行，比如使用默认的Shader etc.
FrameWork::CaIShader* FrameWork::CaIShader::Create(uint32_t &id, const std::string &shaderPath, VkFormat colorFormat) {
    for (int i = 0; i < caiShaderPool.size(); i++) {
        if (caiShaderPool[i] == nullptr) {
            id = i;
            caiShaderPool[i] = new FrameWork::CaIShader(shaderPath, colorFormat);
            if (caiShaderPool[i]->shaderPath != shaderPath) {
                delete caiShaderPool[i];
                caiShaderPool[i] = nullptr;
                throw std::runtime_error("Can't Create Shader By :" + shaderPath);
            }else {
                return caiShaderPool[i];
            }
        }
    }

    caiShaderPool.push_back(new FrameWork::CaIShader(shaderPath, colorFormat));
    if (caiShaderPool.back()->shaderPath != shaderPath) {
        delete caiShaderPool.back();
        caiShaderPool.back() = nullptr;
        throw std::runtime_error("Can't Create Shader By :" + shaderPath);
    }else {
        id = caiShaderPool.size() - 1;
        return caiShaderPool.back();
    }
}

void FrameWork::CaIShader::Destroy(uint32_t &id) {
    if (id < caiShaderPool.size() && caiShaderPool[id] != nullptr) {
        delete caiShaderPool[id];
        caiShaderPool[id] = nullptr;
        return;
    }
    LOG_WARNING("Destroy {} is not existed shader", id);
}

//这个函数不保证线程安全
FrameWork::CaIShader * FrameWork::CaIShader::Get(uint32_t id) {
    if (id < caiShaderPool.size() && caiShaderPool[id] != nullptr) {
        return caiShaderPool[id];
    }
    LOG_ERROR("Trying to access non-existent shader id {}", id);
    return nullptr;
}

FrameWork::CaIShader *FrameWork::CaIShader::Get(const Handle<CaIShader> &handle) {
    if (!handle) {
        LOG_ERROR("Trying to access non-existent CaIShader handle");
        return nullptr;
    }
    auto shader = caiShaderWrappedPool.GetResource(handle.index);
    if (shader == nullptr) {
        LOG_ERROR("CaIShader handle {} is nullptr", handle.index);
    }
    return shader;
}

void FrameWork::CaIShader::DestroyAll() {
    for (auto& shader: caiShaderPool) {
        delete shader;
    }
}

bool FrameWork::CaIShader::exist(uint32_t id) {
    if (id < caiShaderPool.size() && caiShaderPool[id] != nullptr) {
        return true;
    }
    return false;
}



//支持Dynamic Rendering
FrameWork::CaIShader::CaIShader(const std::string &shaderPath, VkFormat colorFormat) {
    auto opShaderInfo = vulkanRenderAPI.CreateVulkanPipeline(pipelineID, shaderPath, colorFormat);
    if (opShaderInfo) {
        shaderInfo = std::move(opShaderInfo.value());
        this->shaderPath = shaderPath;
    }else {
        throw std::runtime_error("CreateVulkanPipeline error " + opShaderInfo.error());
    }
}

FrameWork::CaIShader::~CaIShader() {
    //释放的方式押入释放队列，因为多飞行帧的原因不能立刻释放资源，释放队列由vulkanRenderAPI管理
    if (pipelineID != UINT32_MAX)
        vulkanRenderAPI.DeletePipeline(pipelineID);
}

void * FrameWork::CaIShader::GetShaderPropertyAddress(uint32_t materialDataID, const std::string &name, uint32_t id) {
    auto materialData = vulkanRenderAPI.getByIndex<FrameWork::MaterialData>(materialDataID);
    //根据name遍历找到对应的地址
    for (auto& property :  shaderInfo.vertProperties.baseProperties) {
        if (property.name == name) {
            return reinterpret_cast<char*>
            (materialData->vertexUniformBuffers[vulkanRenderAPI.currentFrame].mapped) + property.offset + property.arrayOffset * id;
        }
    }
    for (auto& property : shaderInfo.fragProperties.baseProperties) {
        if (property.name == name) {
            return reinterpret_cast<char*>
            (materialData->fragmentUniformBuffers[vulkanRenderAPI.currentFrame].mapped) + property.offset + property.arrayOffset * id;
        }
    }
    LOG_ERROR("Can't find shader property name : {} in {}", name, shaderPath);
    return nullptr;
}


void FrameWork::CaIShader::Bind(const VkCommandBuffer &cmdBuffer) const {
    auto pipeline = vulkanRenderAPI.getByIndex<FrameWork::VulkanPipeline>(pipelineID)->pipeline;
    vkCmdBindPipeline(
        cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline
        );
}

FrameWork::ShaderInfo FrameWork::CaIShader::GetShaderInfo() const {
    return shaderInfo;
}

uint32_t FrameWork::CaIShader::GetPipelineID() const {
    return pipelineID;
}


namespace FrameWork
{


    Handle<CaIShader> CaIShader::CreateHandle(const std::string& shaderPath, VkFormat colorFormat) {
        uint32_t index = caiShaderWrappedPool.CreateResource(shaderPath, colorFormat);
        Handle<CaIShader> handle;
        handle.index = index;
        return handle;
    }

    bool CaIShader::Bind(Handle<CaIShader>& handle, const VkCommandBuffer& cmdBuffer){
        if (!handle) {
            LOG_ERROR("Trying to bind a non-existent CaIShader handle");
            return false;
        }
        auto shader = CaIShader::caiShaderWrappedPool.GetResource(handle.index);
        if (shader) {
            shader->Bind(cmdBuffer);
        } else {
            LOG_ERROR("CaIShader with index {} does not exist", handle.index);
            return false;
        }
        return true;
    }
    uint32_t CaIShader::GetRef(uint32_t id) {
        return CaIShader::caiShaderWrappedPool.GetRefNum(id);
    }

    ShaderInfo CaIShader::GetInfo(const Handle<CaIShader>& shaderHandle) {
        return CaIShader::caiShaderWrappedPool.GetResource(shaderHandle.index)->GetShaderInfo();
    }

    uint32_t CaIShader::GetPipelineID(const Handle<CaIShader>& pipelineShader){
        return CaIShader::caiShaderWrappedPool.GetResource(pipelineShader.index)->pipelineID;
    }

    void* CaIShader::GetShaderPropertyAddress(const Handle<CaIShader>& shaderHandle, const Handle<CaIMaterial>& materialHandle, const std::string& name, uint32_t id){
        auto materialData = vulkanRenderAPI.getByIndex<FrameWork::MaterialData>(
            CaIMaterial::GetMaterialDataID(materialHandle)
        );
        auto shaderInfo = CaIShader::GetInfo(shaderHandle);
        auto shaderPath = CaIShader::caiShaderWrappedPool.GetResource(shaderHandle.index)->shaderPath;
        //根据name遍历找到对应的地址
        for (auto& property :  shaderInfo.vertProperties.baseProperties) {
            if (property.name == name) {
                return reinterpret_cast<char*>
                (materialData->vertexUniformBuffers[vulkanRenderAPI.currentFrame].mapped) + property.offset + property.arrayOffset * id;
            }
        }
        for (auto& property : shaderInfo.fragProperties.baseProperties) {
            if (property.name == name) {
                return reinterpret_cast<char*>
                (materialData->fragmentUniformBuffers[vulkanRenderAPI.currentFrame].mapped) + property.offset + property.arrayOffset * id;
            }
        }
        LOG_ERROR("Can't find shader property name : {} in {}", name, shaderPath);
        return nullptr;
    }
}
