//
// Created by 51092 on 25-8-19.
//

#ifndef SCENE_H
#define SCENE_H
#include<vector>

#include "Camera.h"
#include "Light.h"
#include "PublicStruct.h"

namespace FrameWork{
    class Scene {
    public:
        virtual ~Scene() = default;
        virtual void Render(const VkCommandBuffer& cmdBuffer) = 0;
        virtual const std::function<void()>& GetRenderFunction() = 0;
        virtual std::string GetName() const = 0;
    };


}



#endif //SCENE_H
