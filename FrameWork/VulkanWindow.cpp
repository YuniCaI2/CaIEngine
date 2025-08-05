//
// Created by 51092 on 25-6-1.
//

#include "VulkanWindow.h"
#include<iostream>

FrameWork::VulkanWindow& FrameWork::VulkanWindow::GetInstance() {
    static FrameWork::VulkanWindow instance;  // 自动管理生命周期
    return instance;
}

GLFWwindow * FrameWork::VulkanWindow::GetWindow() {
    return window;
}

FrameWork::VulkanWindow::VulkanWindow() {
    //初始化,不然后续关于glfw的操作会报错
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    this->window = glfwCreateWindow(1280, 720, "Start", nullptr, nullptr);
    if (window == nullptr) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    //这边是为了禁用鼠标显示
    glfwSetWindowUserPointer(window, this);
    //还有尺寸调整的回调
}


