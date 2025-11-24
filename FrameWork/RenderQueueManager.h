//
// Created by 51092 on 2025/10/4.
//

#ifndef CAIENGINE_RENDERQUEUEMANAGER_H
#define CAIENGINE_RENDERQUEUEMANAGER_H
#include <memory>
#include "RenderQueue.h"
#include <unordered_map>

namespace FrameWork {
    class RenderQueueManager {
    public:
        RenderQueueManager();
        ~RenderQueueManager();
        void AddDrawItem(std::unique_ptr<DrawItem>&& drawItem, RenderQueueType renderQueueType);
        void AddDrawIntem(std::unique_ptr<DrawItem>&& drawItem, RenderQueue::RenderQueueLevel renderQueueLevel);
        RenderQueue* GetRenderQueue(RenderQueueType renderQueueType);
        RenderQueue* GetRenderQueue(RenderQueue::RenderQueueLevel renderQueueLevel);
        void SortAll(const Camera& camera);
        void ClearAll();

        static RenderQueueManager& GetInstance();
        
    private:
        std::unordered_map<RenderQueue::RenderQueueLevel , std::unique_ptr<RenderQueue>> renderQueueMap;
    };
}


#endif //CAIENGINE_RENDERQUEUEMANAGER_H