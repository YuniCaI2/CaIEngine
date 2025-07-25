//
// Created by 51092 on 25-5-30.
//

#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H
#include<GLFW/glfw3.h>
#include"pubh.h"
#include "PublicEnum.h"
#include<functional>
#include"VulkanTool.h"

//因为我没有打算使用别的窗口库的打算
namespace FrameWork {
    class InputManager {
        struct KeyStates {
            std::vector<int> key; //记录键盘是否被持续按下 -- 记录当前按键的状态
            std::vector<bool> keyPressed; //记录当前帧是否被按下（也就是前一帧按下后这帧就置为false）
            std::vector<bool> keyReleased; //记录当前帧中的按钮是否被释放

            std::vector<bool> lastKeyPressed;

            KeyStates() :
            key(Key_Escape + 1, false), keyPressed(Key_Escape + 1, false), keyReleased(Key_Escape + 1, false),
            lastKeyPressed(Key_Escape + 1, false)
            {}
        } keyStates;

        struct MouseStates {
            int mouseMidButton{false}; //当前按钮状态
            bool mouseMidButtonPressed{false};
            bool mouseMidButtonReleased{false};
            bool lastMouseMidButtonPressed{false};

            int mouseRightButton{false};
            bool mouseRightButtonPressed{false};
            bool mouseRightButtonReleased{false};
            bool lastMouseRightButtonPressed{false};

            int mouseLeftButton{false};
            bool mouseLeftButtonPressed{false};
            bool mouseLeftButtonReleased{false};
            bool lastMouseLeftButtonPressed{false};


            int posX, posY;
            int deltaX, deltaY;
        } mouseStates;

        GLFWwindow* window;

        //GFLW函数的包装器
        static void KeyCallbackInternal(GLFWwindow* window, int key, int scancode, int action, int mods);
        static void MouseButtonCallbackInternal(GLFWwindow* window, int button, int action, int mods);
        static void CursorPosCallbackInternal(GLFWwindow* window, double xpos, double ypos);
        static void ScrollCallbackInternal(GLFWwindow* window, double xoffset, double yoffset);

        InputManager();
    public:

        //因为GLFW的特殊性质，这里要对其进行包装
        using KeyCallback = VulkanTool::KeyCallback;
        using MouseButtonCallback = VulkanTool::MouseButtonCallback;
        using CursorPosCallback = VulkanTool::CursorPosCallback;
        using ScrollCallback = VulkanTool::ScrollCallback;

        //回调函数链
        std::vector<KeyCallback> userKeyCallbacks;
        std::vector<MouseButtonCallback> userMouseButtonCallbacks;
        std::vector<CursorPosCallback> userCursorPosCallbacks;
        std::vector<ScrollCallback> userScrollCallbacks;

        [[nodiscard]] glm::vec2 GetMousePosition() const;

        [[nodiscard]] glm::vec2 GetMouseDelta() const;

        [[nodiscard]] bool GetMouseButton(MouseButton mouseButton) const;

        [[nodiscard]] bool GetMouseButtonPressed(MouseButton mouseButton) const;

        [[nodiscard]] bool GetMouseButtonReleased(MouseButton mouseButton) const;

        [[nodiscard]] bool GetKey(Key keyCode) const;

        [[nodiscard]] bool GetKeyPressed(Key keyCode) const;

        [[nodiscard]] bool GetKeyReleased(Key keyCode) const;

        //这里接受回调函数的指针是为了绕过function无法比较的问题
        void addKeyCallback(KeyCallback&& callback);
        void addMouseButtonCallback(MouseButtonCallback&& callback);
        void addCursorPosCallback(CursorPosCallback&& callback);
        void addScrollCallback(ScrollCallback&& callback);

        static InputManager& GetInstance();

        //更新函数，需要写在渲染循环中
        void update();

    };
}


#endif //INPUTMANAGER_H
