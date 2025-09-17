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


    //pipeline
    uint32_t pipelineID_ = - 1;
    uint32_t slot_ = -1;
    uint32_t frameBufferID_ = -1;
    uint32_t presentColorAttachment_=  -1;

    //Material
    uint32_t materialSlot_ = -1;
    float roughness = 0.01;
    glm::vec3 F0 = {0.03, 0.03, 0.03};
    glm::vec3 diffuse{1.0, 1.0, 1.0};

    struct Material {
        alignas(16) glm::vec3 diffuse{1.0, 1.0, 1.0};
        alignas(16) glm::vec3 F0 = {0.03, 0.03, 0.03};
        alignas(4) float roughness = 0.0;
        void Update(const glm::vec3* diffuse, const glm::vec3* F0, const float* roughness) {
            this->diffuse = *diffuse;
            this->F0 = *F0;
            this->roughness = *roughness;
        }
    };

    //floor
    uint32_t floorID = -1;
    uint32_t lightID = -1;
    float rotate = 0.0f;
    uint32_t floorPipelineID_ = -1;

    //Light
    struct Light {
        alignas(16) glm::mat4 modelMatrix {};
        alignas(16) glm::vec3 position{0,0,0};
        alignas(16) glm::vec3 normal{0.0f, 0.0f, 1.0f};
        alignas(4) float height{1};//初始值固定
        alignas(4) float width{1}; //初始值固定
        alignas(4) float intensity{5};
        alignas(16) glm::vec3 color{1.0f, 1.0f, 1.0f};
        alignas(16) glm::vec3 cameraPosition{0.0f, 0.0f, 0.0f};

        void Update(const glm::vec3* position, const float* lightRotateY, 
            const float* lightRotateX, const float* lightScaleY, const float* lightScaleX, const float* intensity, const glm::vec3* lightColor, const FrameWork::Camera* camera) {
            auto rotateY = glm::rotate(glm::mat4(1.0f), glm::radians(*lightRotateY
        ), glm::vec3(0.0f, 1.0f, 0.0f));
            // std::cout << "intensity:" << *intensity << std::endl;
            auto rotateX = glm::rotate(glm::mat4(1.0f), glm::radians(*lightRotateX), glm::vec3(1.0f, 0.0f, 0.0f));
            auto scale = glm::scale(glm::mat4(1.0f), glm::vec3(*lightScaleX, *lightScaleY, 1.0f));
            auto pos = glm::translate(glm::mat4(1.0f), *position);
            modelMatrix = pos * rotateY * rotateX * scale * glm::mat4(1.0f);
            this->intensity = *intensity;
            color = *lightColor;
            cameraPosition = camera->Position;
        }
    };
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

};



#endif //LTCSCENE_H
