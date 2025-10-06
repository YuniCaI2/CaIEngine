//
// Created by 51092 on 2025/10/4.
//

#ifndef CAIENGINE_RENDERQUEUE_H
#define CAIENGINE_RENDERQUEUE_H
#include<vector>

namespace FrameWork {
    struct DrawItem;
}

namespace FrameWork {
    enum class SortType {
        BackToFront = 0,
        FrontToBack = 1,
    };
    class RenderQueue {
    public:
        using RenderLists = std::vector<std::unique_ptr<DrawItem>>;
        RenderQueue();
        ~RenderQueue();

        void AddDrawItem(std::unique_ptr<DrawItem>&& drawItem);
        RenderLists& GetRenderLists();
        void Clear();
        void SortRenderLists(SortType sortType);
    private:
        RenderLists renderLists;

    };
}


#endif //CAIENGINE_RENDERQUEUE_H