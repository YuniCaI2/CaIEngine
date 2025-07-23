//
// Created by 51092 on 25-6-1.
//

#ifndef VULKANWINDOW_H
#define VULKANWINDOW_H
#include "GLFW/glfw3.h"


//保证窗口全局唯一
namespace FrameWork {
    class VulkanWindow {
    public:
        static VulkanWindow& GetInstance();
        GLFWwindow* GetWindow();
    private:
        VulkanWindow();
        GLFWwindow* window;
    };
}



#endif //VULKANWINDOW_H
