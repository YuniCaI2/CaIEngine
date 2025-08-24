//
// Created by 51092 on 25-8-23.
//

#ifndef LTCSCENE_H
#define LTCSCENE_H
#include "Scene.h"


class LTCScene : public FrameWork::Scene {
public:
    LTCScene(FrameWork::Camera* camera);
    virtual ~LTCScene() override;
    virtual void Render(const VkCommandBuffer& cmdBuffer) override;
    virtual const std::function<void()>& GetRenderFunction() override;
    virtual std::vector<uint32_t> GetModelIDs() override;
    virtual std::string GetName() const override;
    virtual uint32_t GetPresentColorAttachment() override;
private:
    FrameWork::Camera* camera_;
    float intensity_;

    //pipeline
    uint32_t pipelineID_ = - 1;
    uint32_t slot_ = -1;
    uint32_t frameBufferID_ = -1;
    uint32_t presentColorAttachment_=  -1;

    //floor
    uint32_t floorID = -1;
    float rotate = 0.0f;

    std::function<void()> GUIFunc;

};



#endif //LTCSCENE_H
