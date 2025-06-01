//
// Created by 51092 on 25-5-30.
//

#include "InputManager.h"


void FrameWork::InputManager::KeyCallbackInternal(GLFWwindow *window, int key, int scancode, int action, int mods) {
    auto* instance = static_cast<InputManager*>(glfwGetWindowUserPointer(window));
    for (auto& func : instance->userKeyCallbacks) {
        (func)(key, scancode, action, mods);
    }
}

void FrameWork::InputManager::MouseButtonCallbackInternal(GLFWwindow *window, int button, int action, int mods) {
    auto* instance = static_cast<InputManager*>(glfwGetWindowUserPointer(window));
    for (auto& func : instance->userMouseButtonCallbacks) {
        (func)(button, action, mods);
    }
}

void FrameWork::InputManager::CursorPosCallbackInternal(GLFWwindow *window, double xpos, double ypos) {
    auto* instance = static_cast<InputManager*>(glfwGetWindowUserPointer(window));
    for (auto& func : instance->userCursorPosCallbacks) {
        (func)(xpos, ypos);
    }
}

void FrameWork::InputManager::ScrollCallbackInternal(GLFWwindow *window, double xoffset, double yoffset) {
    auto* instance = static_cast<InputManager*>(glfwGetWindowUserPointer(window));
    for (auto& func : instance->userScrollCallbacks) {
        (func)(xoffset, yoffset);
    }
}

FrameWork::InputManager::InputManager(GLFWwindow *window) {
    this->window = window;

    //设置窗口的回调
    glfwSetWindowUserPointer(window, this);
    glfwSetMouseButtonCallback(window, MouseButtonCallbackInternal);
    glfwSetMouseButtonCallback(window, MouseButtonCallbackInternal);
    glfwSetCursorPosCallback(window, CursorPosCallbackInternal);
    glfwSetScrollCallback(window, ScrollCallbackInternal);
    //绑定指针
}

void FrameWork::InputManager::update() {
    glfwPollEvents();

    //Mouse
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    mouseStates.deltaX = (int)xpos - mouseStates.posX;
    mouseStates.deltaY = (int)ypos - mouseStates.posY;


    mouseStates.posX = static_cast<int>(xpos);
    mouseStates.posY = static_cast<int>(ypos);

    mouseStates.mouseLeftButtonPressed = false;
    mouseStates.mouseRightButtonPressed = false;
    mouseStates.mouseMidButtonPressed = false;

    mouseStates.mouseLeftButtonReleased = false;
    mouseStates.mouseRightButtonReleased = false;
    mouseStates.mouseMidButtonReleased = false;

    mouseStates.mouseMidButton = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE);
    mouseStates.mouseLeftButton = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    mouseStates.mouseRightButton = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);

    if (mouseStates.mouseMidButton == GLFW_PRESS && !mouseStates.lastMouseMidButtonPressed) {
        mouseStates.mouseMidButtonPressed = true;
    }

    if (mouseStates.mouseMidButton == GLFW_RELEASE && mouseStates.lastMouseMidButtonPressed) {
        mouseStates.mouseMidButtonReleased = true;
    }

    if (mouseStates.mouseLeftButton == GLFW_PRESS && !mouseStates.lastMouseLeftButtonPressed) {
        mouseStates.mouseLeftButtonPressed = true;
    }

    if (mouseStates.mouseLeftButton == GLFW_RELEASE && mouseStates.lastMouseLeftButtonPressed) {
        mouseStates.mouseLeftButtonReleased = true;
    }

    if (mouseStates.mouseRightButton == GLFW_PRESS && !mouseStates.lastMouseRightButtonPressed) {
        mouseStates.mouseRightButtonPressed = true;
    }

    if (mouseStates.mouseRightButton == GLFW_RELEASE && mouseStates.lastMouseRightButtonPressed) {
        mouseStates.mouseRightButtonReleased = true;
    }

    //记录上一帧的内容
    mouseStates.lastMouseLeftButtonPressed = (mouseStates.mouseLeftButton == GLFW_PRESS);
    mouseStates.lastMouseRightButtonPressed = (mouseStates.mouseRightButton == GLFW_PRESS);
    mouseStates.lastMouseMidButtonPressed = (mouseStates.mouseMidButton == GLFW_PRESS);




    //Key
    for (int i = 0; i < 26; i++) {
        keyStates.key[i + Key_A] = (glfwGetKey(window, GLFW_KEY_A + i));
    }
    for (int i = 0; i < 10; i++) {
        keyStates.key[i + Key_0] = (glfwGetKey(window, GLFW_KEY_0 + i));
    }

    keyStates.key[Key_Right] = (glfwGetKey(window, GLFW_KEY_RIGHT));
    keyStates.key[Key_Left] = (glfwGetKey(window, GLFW_KEY_LEFT));
    keyStates.key[Key_Up] = (glfwGetKey(window, GLFW_KEY_UP));
    keyStates.key[Key_Down] = (glfwGetKey(window, GLFW_KEY_DOWN));

    keyStates.key[Key_Space] = (glfwGetKey(window, GLFW_KEY_SPACE));
    keyStates.key[Key_LeftShift] = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT));
    keyStates.key[Key_RightShift] = (glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT));

    keyStates.key[Key_Escape] = (glfwGetKey(window, GLFW_KEY_ESCAPE));

    for (int i = 0; i < keyStates.key.size(); i++) {
        keyStates.keyPressed[i] = false;
        keyStates.keyReleased[i] = false;

        if (keyStates.key[i] == GLFW_PRESS) {
            if (!keyStates.lastKeyPressed[i]) {
                keyStates.keyPressed[i] = true;
            }
        }
        if (keyStates.key[i] == GLFW_RELEASE && keyStates.lastKeyPressed[i]) {
            keyStates.keyReleased[i] = true;
        }
    }

    //记录上一帧的状态
    for (int i = 0; i < keyStates.key.size(); ++i) {
        keyStates.lastKeyPressed[i] = (keyStates.key[i] == GLFW_PRESS);
    }
}

glm::vec2 FrameWork::InputManager::GetMousePosition() const{
    return {mouseStates.posX, mouseStates.posY};
}

glm::vec2 FrameWork::InputManager::GetMouseDelta() const{
    return {mouseStates.deltaX, mouseStates.deltaY};
}

bool FrameWork::InputManager::GetMouseButton(MouseButton mouseButton) const{
    if (mouseButton == MouseButton::Left) {
        return mouseStates.mouseLeftButton;
    }
    if (mouseButton == MouseButton::Right) {
        return mouseStates.mouseRightButton;
    }
    if (mouseButton == MouseButton::Mid) {
        return mouseStates.mouseMidButton;
    }else {
        std::cerr << "请检查当前传入的鼠标按钮是否合法" << std::endl;
        exit(-1);
    }
}

bool FrameWork::InputManager::GetMouseButtonPressed(MouseButton mouseButton) const{

    if (mouseButton == MouseButton::Left) {
        return mouseStates.mouseLeftButtonPressed;
    }
    if (mouseButton == MouseButton::Right) {
        return mouseStates.mouseRightButtonPressed;
    }
    if (mouseButton == MouseButton::Mid) {
        return mouseStates.mouseMidButtonPressed;
    }else {
        std::cerr << "请检查当前传入的鼠标按钮是否合法" << std::endl;
        exit(-1);
    }

}

bool FrameWork::InputManager::GetMouseButtonReleased(MouseButton mouseButton) const{

    if (mouseButton == MouseButton::Left) {
        return mouseStates.mouseLeftButtonReleased;
    }
    if (mouseButton == MouseButton::Right) {
        return mouseStates.mouseRightButtonReleased;
    }
    if(mouseButton == MouseButton::Mid) {
        return mouseStates.mouseMidButtonReleased;
    }
    else {
        std::cerr << "请检查当前传入的鼠标按钮是否合法" << std::endl;
        exit(-1);
    }

}

bool FrameWork::InputManager::GetKey(Key keyCode) const{

    return keyStates.key[keyCode];
}

bool FrameWork::InputManager::GetKeyPressed(Key keyCode) const{
    return keyStates.keyPressed[keyCode];
}

bool FrameWork::InputManager::GetKeyReleased(Key keyCode) const{
    return keyStates.keyReleased[keyCode];
}

void FrameWork::InputManager::addKeyCallback(KeyCallback&& callback) {
    //加入回调函数
    if (callback != nullptr)
        userKeyCallbacks.push_back(callback);
}

void FrameWork::InputManager::addMouseButtonCallback(MouseButtonCallback&& callback) {
    if (callback != nullptr)
        userMouseButtonCallbacks.push_back(callback);
}

void FrameWork::InputManager::addCursorPosCallback(CursorPosCallback&& callback) {
    if (callback != nullptr)
        userCursorPosCallbacks.push_back(callback);
}

void FrameWork::InputManager::addScrollCallback(ScrollCallback&& callback) {
    if (callback != nullptr)
        userScrollCallbacks.push_back(callback);
}
