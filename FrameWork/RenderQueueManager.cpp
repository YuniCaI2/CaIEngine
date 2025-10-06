//
// Created by 51092 on 2025/10/4.
//

#include "RenderQueueManager.h"
#include "entt/entt.hpp"

FrameWork::RenderQueueManager::~RenderQueueManager() {

}

void FrameWork::RenderQueueManager::AddDrawItem(std::unique_ptr<DrawItem> &&drawItem, RenderQueueType renderQueueType) {
    renderQueues[static_cast<int>(renderQueueType)]->AddDrawItem(std::move(drawItem));
}


FrameWork::RenderQueue * FrameWork::RenderQueueManager::GetRenderQueue(RenderQueueType renderQueueType) {
    return renderQueues[static_cast<size_t>(renderQueueType)].get();
}

void FrameWork::RenderQueueManager::ClearAll() {
    for (auto& queue : renderQueues) {
        queue->Clear();
    }
}

FrameWork::RenderQueueManager::RenderQueueManager() {
    renderQueues.push_back(std::make_unique<RenderQueue>()); // Opaque
    renderQueues.push_back(std::make_unique<RenderQueue>()); //Transparent
}
