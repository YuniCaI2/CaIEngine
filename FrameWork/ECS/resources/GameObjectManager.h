//
// Created by 51092 on 2025/10/8.
//

#ifndef CAIENGINE_GAMEOBJECTMANAGER_H
#define CAIENGINE_GAMEOBJECTMANAGER_H
#include <entt/entt.hpp>

namespace ECS {
    class GameObjectManager {
    public:
        GameObjectManager();
        entt::entity CreateEmptyGameObject(const std::string& name);
        entt::entity LoadPrefabGameObject(const std::string& name, const std::string& prefabPath);
    private:
        entt::registry& registryRef;
    };
}


#endif //CAIENGINE_GAMEOBJECTMANAGER_H