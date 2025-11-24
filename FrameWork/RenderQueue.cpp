//
// Created by 51092 on 2025/10/4.
//

#include "RenderQueue.h"
#include <algorithm>
#include <entt/entt.hpp>

FrameWork::RenderQueue::RenderQueue(RenderQueueLevel renderQueueLevel) {
    this->renderQueueLevel = renderQueueLevel;
}

FrameWork::RenderQueue::RenderQueue(FrameWork::RenderQueueType renderQueueType){
    this->renderQueueLevel = static_cast<RenderQueueLevel>(renderQueueType);
}

FrameWork::RenderQueue::~RenderQueue() {
    Clear();
}

void FrameWork::RenderQueue::AddDrawItem(std::shared_ptr<DrawItem> drawItem) {
    std::lock_guard<std::mutex> lock(listsMutex);
    renderLists[drawItem->passName].push_back(drawItem);
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

void FrameWork::RenderQueue::MakeSortKey(std::shared_ptr<DrawItem>& drawItem, const Camera& camera) {
    const uint64_t p = uint64_t(drawItem->pipelineHandle.index & 0xFFFF);   // 明确限定 16 位
    const uint64_t m = uint64_t(drawItem->materialHandle.index & 0xFFFF);   // 明确限定 16 位

    if (renderQueueLevel < 2450) {
        // 不透明：管线 -> 材质 -> 深度（低 32 位）
        const uint64_t d = uint64_t(drawItem->depth);             // 假设已有 32 位深度
        drawItem->sortKey = (p << 48) | (m << 32) | d;
    } else {
        // 半透明：深度 -> 管线 -> 材质
        // 视线方向上的投影距离（不是欧氏距离），[-1000,1000] 映射到 [0,1]
        float viewDepth = glm::dot(camera.Front, drawItem->position - camera.Position);
        float depthNorm = (viewDepth + 1000.0f) / 2000.0f;
        depthNorm = std::clamp(depthNorm, 0.0f, 1.0f);
        auto depth16 = static_cast<uint16_t>(depthNorm * 65535.0f + 0.5f);
        auto keyDepth = depth16;

        drawItem->depth = depth16; // 保存量化后的深度（若需要）
        drawItem->sortKey = (uint64_t(keyDepth) << 48) | (p << 32) | m;
    }
}