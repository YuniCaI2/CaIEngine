//
// Created by 51092 on 25-6-13.
//

#ifndef RESOURCE_H
#define RESOURCE_H
#include <string>
#include "pubh.h"

#include "PublicEnum.h"
namespace FrameWork {
    //这个类的作用是将外部的资源加载或者转为程序可用的资源
    class Resource {
    private:
        std::string resourcePath{"../resources/"};
        std::string generalShaderPath{"../resources/shaders/glsl/"};
    public:
        Resource();
        VkShaderModule getShaderModulFromFile(VkDevice device,const std::string& fileName, VkShaderStageFlags stage) const;
    };
}

#endif //RESOURCE_H
