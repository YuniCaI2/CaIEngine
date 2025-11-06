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

    //约定RenderQueueLevel的值
    //1. Background (1000): 最先渲染的队列，通常用于天空盒。
    //2. Geometry (2000): 默认队列，用于不透明的几何体。
    //3. AlphaTest (2450): 用于透明度测试的几何体。
    //4. Transparent (3000): 渲染透明物体，按照从后到前的顺序。
    //5. Overlay (4000): 最后渲染的队列，适用于叠加效果(如镜头光晕)。

    class RenderQueue {
    public:
        using RenderQueueLevel = uint32_t;
        //shaderTag -> renderQueue
        using RenderLists = std::unordered_map<std::string, std::vector<std::shared_ptr<DrawItem>>>;
        RenderQueue(RenderQueueLevel renderQueueLevel);
        ~RenderQueue();
        void AddDrawItem(std::shared_ptr<DrawItem> drawItem);
        RenderLists& GetRenderLists();
        void Clear();
        void SortRenderLists(const Camera& camera,SortType sortType);
    private:
        void MakeSortKey(std::shared_ptr<DrawItem>& drawItem, const Camera& camera);
        std::mutex listsMutex;
        RenderLists renderLists;
        RenderQueueLevel renderQueueLevel{};
    };
}


#endif //CAIENGINE_RENDERQUEUE_H