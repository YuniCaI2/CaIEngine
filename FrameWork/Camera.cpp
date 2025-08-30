//
// Created by 51092 on 25-8-6.
//
#include "Camera.h"

#include "vulkanFrameWork.h"

namespace FrameWork {
    void Intersection::RegisterModel(uint32_t model) {
        modelIDs.push_back(model);
    }

    uint32_t Intersection::GetIntersectingModel() const {
        return intersectingModel;
    }

    void Intersection::Update(const RayCast &rayCast) {

    }

    uint32_t Intersection::GetIntersectingModelAABB(const RayCast& rayCast) const {
        uint32_t rt = -1;
        int minLen = -1;
        for (auto& m : modelIDs) {
            auto model = vulkanRenderAPI.getByIndex<FrameWork::Model>(m);
            auto trueRayCast = rayCast;
            trueRayCast.origin -= model->position; //相对移动
            if (auto len = model->aabb.InterSectingExtent(rayCast); len != -1) {
                if (minLen > len) {
                    minLen = len;
                    rt = m;
                }
            }
        }
        return rt;
    }

    void Intersection::RecreateInputBuffer() {
        if (InputAABB.buffer == VK_NULL_HANDLE) {

        }
    }
}
