//
// Created by 51092 on 2025/10/8.
//

#ifndef CAIENGINE_GAMEOBJECTMANAGER_H
#define CAIENGINE_GAMEOBJECTMANAGER_H
#include <entt/entt.hpp>
#include <future>

#include "../../PublicEnum.h"

namespace FrameWork {
    struct PrefabStruct;
}

namespace ECS {
    class GameObjectManager {
    public:
        GameObjectManager(entt::registry& registryRef);
        entt::entity CreateEmptyGameObject(const std::string& name);
        ExpectedWithInfo<entt::entity> LoadPrefabGameObject(const std::string& prefabPath);

        //Async

    private:
        ExpectedWithInfo<entt::entity> LoadPrefab(FrameWork::PrefabStruct* prefab);
        entt::registry& registryRef;
    };
}


#endif //CAIENGINE_GAMEOBJECTMANAGER_H