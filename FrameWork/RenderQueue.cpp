//
// Created by 51092 on 2025/10/4.
//

#include "RenderQueue.h"
#include <algorithm>
#include <entt/entt.hpp>

FrameWork::RenderQueue::RenderQueue(RenderQueueType renderQueueType):renderQueueType(renderQueueType) {
}

FrameWork::RenderQueue::~RenderQueue() {
    Clear();
}

void FrameWork::RenderQueue::AddDrawItem(std::unique_ptr<DrawItem> &&drawItem) {
    std::lock_guard<std::mutex> lock(listsMutex);
    renderLists[drawItem->passName].push_back(std::move(drawItem));
}

FrameWork::RenderQueue::RenderLists & FrameWork::RenderQueue::GetRenderLists() {
    return renderLists;
}

void FrameWork::RenderQueue::Clear() {
    renderLists.clear();
}

void FrameWork::RenderQueue::SortRenderLists(const Camera& camera, SortType sortType) {
    std::lock_guard<std::mutex> lock(listsMutex);
    for (auto& [str, renderList] : renderLists) {
        for (auto& drawItem : renderList) {
            MakeSortKey(drawItem, camera);
        }
    }
    if (sortType == SortType::BackToFront) {
        for (auto& [_, renderList] : renderLists) {
            std::ranges::sort(renderList, [](auto& lhs, auto& rhs) {
                return lhs->sortKey > rhs->sortKey;
            });
        }
    }else if (sortType == SortType::FrontToBack) {
        for (auto& [_, renderList] : renderLists) {
            std::ranges::sort(renderList, [](auto& lhs, auto& rhs) {
                return lhs->sortKey < rhs->sortKey;
            });
        }
    }
}

void FrameWork::RenderQueue::MakeSortKey(std::unique_ptr<DrawItem> &drawItem, const Camera& camera) {
    if (renderQueueType == RenderQueueType::Opaque) {
        drawItem->sortKey = (uint64_t)drawItem->pipelineID << 48
        | (uint64_t)drawItem->materialID << 32
        | (uint64_t)drawItem->depth;
    }else if (renderQueueType == RenderQueueType::Transparent) {
        auto depth = (glm::dot(camera.Front, drawItem->position - camera.Position) + 1000.0f) / 2000.0f;
        drawItem->depth = depth * UINT16_MAX;
        drawItem->depth = static_cast<uint32_t>(glm::dot(camera.Front, drawItem->position - camera.Position));
        drawItem->sortKey = (uint64_t)drawItem->depth << 48
        | (uint64_t)drawItem->pipelineID << 32
        | (uint64_t)drawItem->materialID;
    }
}
