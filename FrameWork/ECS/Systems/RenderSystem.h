//
// Created by 51092 on 2025/10/8.
//

#ifndef CAIENGINE_RENDERSYSTEM_H
#define CAIENGINE_RENDERSYSTEM_H

#include "../../Camera.h"
#include "../../FrameGraph/FrameGraph.h"
#include <entt/entt.hpp>

namespace ECS {
    class RenderSystem {
    public:
        static void Render(entt::registry& registry,
            FG::FrameGraph& frameGraph, FrameWork::Camera& camera);
        static void RecordCommandBuffer(VkCommandBuffer commandBuffer);
    private:
        inline static FG::FrameGraph* frameGraphPtr{};
    };
}

#endif //CAIENGINE_RENDERSYSTEM_H