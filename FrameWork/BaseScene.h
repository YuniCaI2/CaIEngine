//
// Created by 51092 on 25-8-19.
//

#ifndef BASESCENE_H
#define BASESCENE_H
#include "Camera.h"
#include "FrameGraph/ResourceManager.h"
#include "Scene.h"
#include "VulkanDebug.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.h>

class BaseScene : public FrameWork::Scene {
  public:
    BaseScene(FrameWork::Camera &camera);
    virtual ~BaseScene() override;
    virtual void Render(const VkCommandBuffer &cmdBuffer) override;
    virtual const std::function<void()> &GetRenderFunction() override;
    virtual std::string GetName() const override;

  private:
    void CreateFrameGraphResource();

    std::string sceneName{};

    // Pipeline资源
    uint32_t pipelineID = -1;
    uint32_t frameBufferID = -1;
    uint32_t globalSlotID = -1;
    std::vector<uint32_t> modelID;
    uint32_t presentColorAttachment = -1;
    // SSAA Resource
    float ssaa = 1.0f;

    // Vulkan对象
    VkDescriptorSetLayout dynamicDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout textureDescriptorSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;

    // 场景数据
    bool useMSAA = false;
    FrameWork::Camera *cameraPtr{};

    FrameWork::AABBDeBugging aabbDeBugging{};
    bool displayAABB = false;

    std::function<void()> GUIFunc; // 设置GUI函数，在Renderer中与GUI对象交互

    // FrameGraph设置
    uint32_t vulkanModelDataIndex = -1;
    std::unique_ptr<FG::FrameGraph> frameGraph;
    // 持久资源
    uint32_t vulkanModelID = -1;
    FrameWork::Handle<FrameWork::CaIMaterial> presentMaterialHandle;
    FrameWork::Handle<FrameWork::CaIMaterial> resolveMaterialHandle;
    // 这边是一个模型对应一个Material
    FrameWork::Handle<FrameWork::CaIShader> shaderHandle;
    std::vector<FrameWork::Handle<FrameWork::CaIMaterial>> materialHandles;
    FrameWork::Handle<FrameWork::CaIShader> presentShaderHandle;
    FrameWork::Handle<FrameWork::CaIShader> resolveShaderHandle;

    uint32_t colorAttachment = 0;
    uint32_t depthAttachment = 0;
    uint32_t resolveAttachment = 0;
    uint32_t bloomingAttachment = 0;
    uint32_t swapChainAttachment = 0;
    std::vector<uint32_t> generateMipAttachments;
};

#endif // BASESCENE_H
