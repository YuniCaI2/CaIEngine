//
// Created by 51092 on 25-8-19.
//

#include "DotLight.h"

float FrameWork::DotLight::GetIntensity() {
    return intensity;
}

glm::mat4 FrameWork::DotLight::GetLightMatrix() {
    return lightMatrix;
}

glm::vec3 FrameWork::DotLight::GetPosition() {
    return position;
}

LightType FrameWork::DotLight::GetType() {
    return type;
}

glm::vec3 FrameWork::DotLight::GetColor() {
    return color;
}

void FrameWork::DotLight::InitLight(const glm::vec3 &position, const glm::vec3 &color, float intensity) {
    this->position = position;
    this->color = color;

}
