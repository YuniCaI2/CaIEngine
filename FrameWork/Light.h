//
// Created by 51092 on 25-8-19.
//

#ifndef LIGHT_H
#define LIGHT_H
#include <glm/glm.hpp>
#include "PublicEnum.h"

namespace FrameWork {
class Light {
public:
    virtual void InitLight(
        const glm::vec3& position, const glm::vec3& color, float intensity
        ) = 0;
    virtual glm::mat4 GetLightMatrix() = 0;
    virtual float GetIntensity() = 0;
    virtual glm::vec3 GetColor() = 0;
    virtual glm::vec3 GetPosition() = 0;
    virtual LightType GetType() = 0;
    virtual ~Light();
};
}



#endif //LIGHT_H
