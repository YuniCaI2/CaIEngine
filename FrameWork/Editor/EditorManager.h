//
// Created by 51092 on 2025/11/1.
//

#ifndef CAIENGINE_EDITORMANAGER_H
#define CAIENGINE_EDITORMANAGER_H
#include<string>

#include "../VulkanBuffer.h"

namespace Editor {
    class EditorManager {
    public:
        void RenderEditor(VkCommandBuffer commandBuffer);
    private:
        //执行在NewFrame之后
        void BuildDockLayout();
        std::string mainDockSpaceName {"mainDockSpaceName"};

    };
}


#endif //CAIENGINE_EDITORMANAGER_H