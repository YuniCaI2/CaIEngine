//
// Created by cai on 2025/10/5.
//

#ifndef CAIENGINE_RENDERABLE_H
#define CAIENGINE_RENDERABLE_H
#include<iostream>
#include "../../PublicEnum.h"
#include "../../RenderQueue.h"

using namespace FrameWork;
namespace ECS {
    struct Renderable {
        std::string passName{};
        uint32_t meshID = 0;
        uint32_t materialID = 0;
        RenderQueueType queueType{RenderQueueType::Opaque};
    };
}
#endif //CAIENGINE_RENDERABLE_H