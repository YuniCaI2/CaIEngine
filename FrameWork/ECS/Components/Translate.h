//
// Created by cai on 2025/10/5.
//

#ifndef CAIENGINE_TRANSLATE_H
#define CAIENGINE_TRANSLATE_H
#include<glm/glm.hpp>
#include <nlohmann/json.hpp>
namespace ECS {
    struct Translate {
        glm::vec3 position{};
        glm::vec3 scale{};
        glm::mat4 rotation{}; //因为rotate有点复杂
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Translate, position, scale, rotation)
}

#endif //CAIENGINE_TRANSLATE_H