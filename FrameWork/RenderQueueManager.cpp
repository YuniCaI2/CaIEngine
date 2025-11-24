//
// Created by 51092 on 2025/10/4.
//

#include "RenderQueueManager.h"
#include "entt/entt.hpp"

FrameWork::RenderQueueManager::~RenderQueueManager() {
}

void FrameWork::RenderQueueManager::AddDrawItem(std::unique_ptr<DrawItem> &&drawItem, RenderQueueType renderQueueType) {
    //因为RenderQueue没有默认构造，这里避免使用[]访问
    if(renderQueueMap.contains(static_cast<size_t>(renderQueueType)) == false) {
        renderQueueMap.emplace(static_cast<RenderQueue::RenderQueueLevel>(renderQueueType), std::make_unique<RenderQueue>(renderQueueType));
    }
}


FrameWork::RenderQueue * FrameWork::RenderQueueManager::GetRenderQueue(RenderQueueType renderQueueType) {
    return renderQueueMap.at(static_cast<RenderQueue::RenderQueueLevel>(renderQueueType)).get();
}

void FrameWork::RenderQueueManager::SortAll(const Camera &camera) {
    for (auto& [renderQueueLevel, queue] : renderQueueMap) {
        if(renderQueueLevel < 2500){
            queue->SortRenderLists(camera, SortType::FrontToBack);
        }
    }
}


void FrameWork::RenderQueueManager::ClearAll() {
    for (auto& [_, queue] : renderQueueMap) {
        queue->Clear();
    }
}

FrameWork::RenderQueueManager& FrameWork::RenderQueueManager::GetInstance() {
    static RenderQueueManager instance;
    return instance;
}

FrameWork::RenderQueueManager::RenderQueueManager() {
}
