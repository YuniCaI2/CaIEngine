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

class BaseScene : public FrameWork::Scene{
public:

    BaseScene(FrameWork::Camera& camera);
    virtual ~BaseScene() override;
    virtual void Render(const VkCommandBuffer& cmdBuffer) override;
    virtual const std::function<void()>& GetRenderFunction() override;
    virtual std::vector<uint32_t> GetModelIDs() override;
    virtual std::string GetName() const override;
    virtual uint32_t GetPresentColorAttachment() override;

private:
    void CreateDescriptorSetLayout();
    void CreateGraphicsPipeline();
    void PrepareResources(FrameWork::Camera& camera);

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

};



#endif //BASESCENE_H
