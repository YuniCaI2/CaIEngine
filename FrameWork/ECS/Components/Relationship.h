//
// Created by 51092 on 2025/10/8.
//

#ifndef CAIENGINE_CHILDREN_H
#define CAIENGINE_CHILDREN_H
#include<entt/entt.hpp>

namespace ECS {
    struct Relationship {
        entt::entity parent;
        std::vector<entt::entity> entities;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Relationship, parent, entities)
}

#endif //CAIENGINE_CHILDREN_H