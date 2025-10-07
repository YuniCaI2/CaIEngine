//
// Created by 51092 on 2025/10/4.
//

#ifndef CAIENGINE_RENDERQUEUE_H
#define CAIENGINE_RENDERQUEUE_H
#include<vector>
#include<iostream>

#include "Camera.h"
#include"PublicStruct.h"

namespace FrameWork {
    enum class SortType {
        BackToFront = 0,
        FrontToBack = 1,
    };
    class RenderQueue {
    public:
        using RenderLists = std::unordered_map<std::string, std::vector<std::unique_ptr<DrawItem>>>;
        using MutexLists = std::unordered_map<std::string, std::unique_ptr<std::mutex>>;
        RenderQueue(RenderQueueType renderQueueType);
        ~RenderQueue();

        void AddDrawItem(std::unique_ptr<DrawItem>&& drawItem);
        RenderLists& GetRenderLists();
        void Clear();
        void SortRenderLists(const Camera& camera,SortType sortType);
    private:
        void MakeSortKey(std::unique_ptr<DrawItem>& drawItem, const Camera& camera);
        std::mutex listsMutex;
        RenderLists renderLists;
        RenderQueueType renderQueueType;
    };
}


#endif //CAIENGINE_RENDERQUEUE_H