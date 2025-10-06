//
// Created by 51092 on 2025/10/4.
//

#ifndef CAIENGINE_RENDERQUEUEMANAGER_H
#define CAIENGINE_RENDERQUEUEMANAGER_H
#include<vector>
#include "RenderQueue.h"
#include "PublicEnum.h"

namespace FrameWork {
    class RenderQueueManager {
    public:
        ~RenderQueueManager();
        void AddDrawItem(std::unique_ptr<DrawItem>&& drawItem, RenderQueueType renderQueueType);
        RenderQueue* GetRenderQueue(RenderQueueType renderQueueType);
        void ClearAll();
    private:
        RenderQueueManager();
        std::vector<std::unique_ptr<RenderQueue>> renderQueues;
    };
}


#endif //CAIENGINE_RENDERQUEUEMANAGER_H