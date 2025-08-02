//
// Created by 51092 on 25-8-2.
//

#ifndef FRAMEWORKGUI_H
#define FRAMEWORKGUI_H
#include "vulkanFrameWork.h"



namespace FrameWork {
    class FrameWorkGUI {
    private:
        //私有资源——为了保证解耦性和独立性，单独创建资源
        VkDescriptorPool descriptorPool{VK_NULL_HANDLE};
        //直接和renderAPI主线程公用CommandBuffer
        VkRenderPass renderPass{VK_NULL_HANDLE};
        std::vector<VkFramebuffer> framebuffers;
        VkPipelineCache pipelineCache{VK_NULL_HANDLE};
        std::function<void()> ItemFunc = nullptr;
        VkDevice device {};
    public:
        void ReleaseGUIResources() const;
        void InitFrameWorkGUI();
        void SetGUIItems(const std::function<void()>& Func);
        void SetGUIItems(std::function<void()>&& Func);
        void RenderGUI(VkCommandBuffer commandBuffer) const;
    };

}



#endif //FRAMEWORKGUI_H
