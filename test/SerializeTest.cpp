//
// Created by 51092 on 2025/10/9.
//

#include <Serialize.h>

#include "Logger.h"
#include "ECS/Components/Renderable.h"
#include "ECS/Components/Translate.h"

struct TestComponent {
    int x;
    int y;
    int z;
};



int main() {
    ECS::Translate translate;
    translate.rotation[0][0] = 1;
    translate.rotation[0][1] = 1;
    translate.rotation[0][2] = 1;
    translate.rotation[0][3] = 1;
    ECS::Renderable renderable;
    nlohmann::json render = renderable;
    nlohmann::json trans = translate;
    nlohmann::json vec3 = glm::vec3(1,1,1);
    std::cout << render << std::endl;
}