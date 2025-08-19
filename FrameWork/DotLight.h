//
// Created by 51092 on 25-8-19.
//

#ifndef SPOTLIGHT_H
#define SPOTLIGHT_H
#include "Light.h"


namespace FrameWork {
class DotLight : public Light{
public:
    virtual float GetIntensity() override;
    virtual glm::mat4 GetLightMatrix() override;
    virtual glm::vec3 GetPosition() override;
    virtual LightType GetType() override;
    virtual glm::vec3 GetColor() override;
    virtual ~DotLight() override = default;
    virtual void InitLight(const glm::vec3 &position, const glm::vec3 &color, float intensity) override;
private:
    float intensity {0.5f};
    glm::vec3 position{};
    glm::mat4 lightMatrix{};
    glm::vec3 color{};
    LightType type{LightType::DOT};
};
}



#endif //SPOTLIGHT_H
