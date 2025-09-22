//
// Created by 51092 on 25-8-23.
//

#ifndef LTCSCENE_H
#define LTCSCENE_H
#include "Scene.h"
#include "FrameGraph/FrameGraph.h"


class LTCScene : public FrameWork::Scene {
public:
    LTCScene(FrameWork::Camera* camera);
    virtual ~LTCScene() override;
    virtual void Render(const VkCommandBuffer& cmdBuffer) override;
    virtual const std::function<void()>& GetRenderFunction() override;
    virtual std::string GetName() const override;
private:
    FrameWork::Camera* camera_;



    //Material
    float roughness = 0.01;
    glm::vec3 F0 = {0.03, 0.03, 0.03};
    glm::vec3 diffuse{1.0, 1.0, 1.0};
    float rotate = 0.0f;

    glm::vec3 lightPos{0, 0.6, 0};
    glm::vec3 lightColor{1.0, 1.0, 1.0};
    float lightRotateY = 0.0f;
    float lightRotateX = 0.0f;
    float lightScaleY = 1.0f;
    float lightScaleX = 1.0f;
    float intensity_ = 5.0f;
    uint32_t lightSlot_ = - 1;

    //LightTex
    uint32_t lightTexID = -1;

    //LTC
    uint32_t LTCTex1ID_ = -1;
    uint32_t LTCTex2ID_ = -1;
    uint32_t LTCSlot_ = -1;

    std::function<void()> GUIFunc;

    //FrameGraph Resource
    std::unique_ptr<FG::FrameGraph> frameGraph;
    FG::RenderPassManager renderPassManager;
    FG::ResourceManager resourceManager;
    void CreateFrameGraphResource();

    uint32_t ltcFaceShaderID = -1;
    uint32_t ltcLightShaderID = -1;
    uint32_t presentShaderID = -1;

    uint32_t ltcFaceMaterialID = -1;
    uint32_t ltcLightMaterialID = -1;
    uint32_t presentMaterialID = -1;

    uint32_t ltcFaceModelID = -1;
    uint32_t ltcLightModelID = -1;

    //各个节点ID
    //PASS
    uint32_t ltcFacePass = -1;
    uint32_t ltcLightPass = -1;
    uint32_t presentPass = -1;
    //Resource
    uint32_t colorAttachmentID = -1;
    uint32_t depthAttachmentID = -1;
    uint32_t colorAttachmentID1 = -1;
    uint32_t depthAttachmentID1 = -1;
    uint32_t swapChainAttachmentID = -1;

};



#endif //LTCSCENE_H
