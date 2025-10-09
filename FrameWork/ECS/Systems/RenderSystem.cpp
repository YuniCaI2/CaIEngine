//
// Created by 51092 on 2025/10/8.
//
#include"RenderSystem.h"
#include "../../RenderQueueManager.h"
#include "../Components/Relationship.h"
#include "../Components/Renderable.h"
#include "../Components/Translate.h"

void ECS::RenderSystem::Render(entt::registry &registry, FG::FrameGraph &frameGraph, FrameWork::Camera &camera) {
    frameGraphPtr = &frameGraph;
    auto& renderQueueManager = FrameWork::RenderQueueManager::GetInstance();
    auto view = registry.view<Renderable, Translate>();
    for (auto& entity : view) {
        auto& renderable = view.get<Renderable>(entity);
        auto& translation = view.get<Translate>(entity);
        auto drawItem = std::make_unique<FrameWork::DrawItem>();
        drawItem->position = translation.position;
        auto scale = glm::scale(glm::mat4(1.0f), translation.scale);
        auto translate = glm::translate(glm::mat4(1.0f), translation.position);
        drawItem->modelMatrix = translate * translation.rotation * scale;
        drawItem->materialID = renderable.materialID;
        drawItem->pipelineID = FrameWork::CaIMaterial::Get(renderable.materialID)->GetShaderID();
        drawItem->meshID = renderable.meshID;
        renderQueueManager.AddDrawItem(std::move(drawItem), renderable.queueType);
    }
    renderQueueManager.SortAll(camera);
}

void ECS::RenderSystem::RecordCommandBuffer(VkCommandBuffer commandBuffer) {
    frameGraphPtr->Execute(commandBuffer);
}
