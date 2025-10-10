//
// Created by 51092 on 2025/10/8.
//

#include "GameObjectManager.h"

#include "../../Logger.h"
#include "../Components/Tag.h"
#include "../../Resource.h"

ECS::GameObjectManager::GameObjectManager(entt::registry& res): registryRef(res) {
}

entt::entity ECS::GameObjectManager::CreateEmptyGameObject(const std::string &name) {
    entt::entity object = registryRef.create();
    registryRef.emplace<Tag>(object, name);
    return object;
}

ExpectedWithInfo<entt::entity> ECS::GameObjectManager::LoadPrefabGameObject(const std::string &prefabPath) {
    entt::entity object = registryRef.create();
    auto expectPrefab = FrameWork::Resource::GetInstance().LoadPrefabStruct(prefabPath);
    if (!expectPrefab) {
        return std::unexpected(ErrorInfo{
            "Load Prefab Error :" + expectPrefab.error()
        });
    }

    auto prefab = std::move(expectPrefab.value());
    return {};
}

ExpectedWithInfo<entt::entity> ECS::GameObjectManager::LoadPrefab(FrameWork::PrefabStruct *prefab) {
    entt::entity object = registryRef.create();
    return {};
}
