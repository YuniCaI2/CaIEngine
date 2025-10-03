//
// Created by 51092 on 25-8-19.
//

#ifndef BASESCENE_H
#define BASESCENE_H
#include "Scene.h"
#include "Camera.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.h>
#include "VulkanDebug.h"
#include "FrameGraph/RenderPassManager.h"
#include "FrameGraph/ResourceManager.h"
#include "FrameGraph/UniformPass/BloomingPass.h"
#include "FrameGraph/UniformPass/DownSamplingPass.h"

class BaseScene : public FrameWork::Scene{
public:

    BaseScene(FrameWork::Camera& camera);
    virtual ~BaseScene() override;
    virtual void Render(const VkCommandBuffer& cmdBuffer) override;
    virtual const std::function<void()>& GetRenderFunction() override;
    virtual std::string GetName() const override;

private:
    void PrepareResources(FrameWork::Camera& camera);
    void CreateFrameGraphResource();

    std::string sceneName{};

    // Pipeline资源
    uint32_t pipelineID = -1;
    uint32_t frameBufferID = -1;
    uint32_t globalSlotID = -1;
    std::vector<uint32_t> modelID;
    uint32_t presentColorAttachment = -1;


    //MSAA Resource
    uint32_t msaaPipelineID = -1;
    uint32_t msaaFrameBufferID = -1;

    //SSAA Resource
    float ssaa = 1.0f;

    // Vulkan对象
    VkDescriptorSetLayout dynamicDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout textureDescriptorSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;

    
    // 场景数据
    bool useMSAA = false;
    FrameWork::Camera* cameraPtr{};

    FrameWork::AABBDeBugging aabbDeBugging{};
    bool displayAABB = false;


    std::function<void()> GUIFunc;//设置GUI函数，在Renderer中与GUI对象交互

    //FrameGraph设置
    uint32_t vulkanModelDataIndex = -1;
    FG::ResourceManager resourceManager;
    FG::RenderPassManager renderPassManager;
    std::unique_ptr<FG::FrameGraph> frameGraph;
    //持久资源
    uint32_t vulkanModelID = -1;
    uint32_t presentMaterialID = -1;
    uint32_t resolveMaterialID = -1; // 本质和present
    //这边是一个模型对应一个Material
    uint32_t caiShaderID = -1;
    std::vector<uint32_t> materials; //这个对应每个mesh
    uint32_t presentShaderID = -1;
    uint32_t resolveShaderID = -1;
    uint32_t testShader = -1;
    uint32_t compShaderID = -1;


    uint32_t colorAttachment = 0;
    uint32_t depthAttachment = 0;
    uint32_t resolveAttachment = 0;
    uint32_t bloomingAttachment = 0;
    uint32_t swapChainAttachment = 0;
    std::vector<uint32_t> generateMipAttachments;
    std::vector<uint32_t> compMaterials;//和pass对应起来
};



#endif //BASESCENE_H
