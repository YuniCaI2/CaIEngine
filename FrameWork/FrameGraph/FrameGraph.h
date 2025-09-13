//
// Created by cai on 2025/9/13.
//

#ifndef CAIENGINE_FRAMEGRAPH_H
#define CAIENGINE_FRAMEGRAPH_H
#include<iostream>

namespace FG {
    class FrameGraph {
    public:
        //仅仅只是将RenderPass和Resource注入给FrameGraph管理
        void AddResourceNode(uint32_t resourceNode);
        void AddRenderPassNode(uint32_t renderPassNode);
        void Compile();
        void Execute();
    private:

        std::vector<uint32_t> resourceNodes;
        std::vector<uint32_t> renderPassNodes;
    };
}


#endif //CAIENGINE_FRAMEGRAPH_H