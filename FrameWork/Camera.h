#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "InputManager.h"
#include "Locator.h"

// 定义摄像机移动的几种可能选项。用作抽象层以避免依赖特定窗口系统的输入方法
enum Camera_Movement {
    FORWARD,    // 前进
    BACKWARD,   // 后退
    LEFT,       // 左移
    RIGHT       // 右移
};

// 默认摄像机参数值
const float YAW         = -90.0f;   // 偏航角（左右旋转）
const float PITCH       =  0.0f;    // 俯仰角（上下旋转）
const float SPEED       =  1.0f;    // 移动速度
const float SENSITIVITY =  0.1f;    // 鼠标灵敏度
const float ZOOM        =  65.0f;   // 视场角度（FOV）


// 抽象摄像机类，处理输入并计算相应的欧拉角、向量和矩阵，可用于OpenGL和Vulkan
namespace FrameWork {
    class Camera
    {
    public:
        // 摄像机属性
        glm::vec3 Position;     // 摄像机位置
        glm::vec3 Front;        // 摄像机前方向量
        glm::vec3 Up;           // 摄像机上方向量
        glm::vec3 Right;        // 摄像机右方向量
        glm::vec3 WorldUp;      // 世界坐标系上方向量

        //实际配合窗口参数
        bool firstMouse{true};
        float lastX{}, lastY{};


        // 欧拉角
        float Yaw;              // 偏航角
        float Pitch;            // 俯仰角

        // 摄像机选项
        float MovementSpeed;    // 移动速度
        float MouseSensitivity; // 鼠标灵敏度
        float Zoom;             // 缩放/视场角

        // 使用向量的构造函数
        Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH)
            : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
        {
            Position = position;
            WorldUp = up;
            Yaw = yaw;
            Pitch = pitch;
            updateCameraVectors();

            //注册回调

            Locator::GetService<InputManager>()->addCursorPosCallback(
            [this](double xpos, double ypos) {
                //设置相机的指针回调
                float xPos = static_cast<float>(xpos);
                float yPos = static_cast<float>(ypos);
                if (firstMouse) {
                    lastX = xPos;
                    lastY = yPos;
                    firstMouse = false;
                }
                auto xOffset = xPos - lastX;
                auto yOffset = lastY - ypos;

                lastX = xPos;
                lastY = yPos;
                ProcessMouseMovement(xOffset, yOffset);
            });
        }

        // 使用标量值的构造函数
        Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch)
            : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
        {
            Position = glm::vec3(posX, posY, posZ);
            WorldUp = glm::vec3(upX, upY, upZ);
            Yaw = yaw;
            Pitch = pitch;
            updateCameraVectors();
        }

        // 返回使用欧拉角和LookAt矩阵计算的视图矩阵
        glm::mat4 GetViewMatrix() const
        {
            return glm::lookAt(Position, Position + Front, Up); // Position 是摄像机在世界坐标系中的位置
            //位置，看向的点，向上的向量
        }

        // 处理从任何类似键盘的输入系统接收的输入。接受摄像机定义的枚举形式的输入参数（用于抽象窗口系统）
        void ProcessKeyboard(Camera_Movement direction, float deltaTime)
        {
            float velocity = MovementSpeed * deltaTime;
            if (direction == FORWARD)
                Position += Front * velocity;
            if (direction == BACKWARD)
                Position -= Front * velocity;
            if (direction == LEFT)
                Position -= Right * velocity;
            if (direction == RIGHT)
                Position += Right * velocity;
        }

        // 处理从鼠标输入系统接收的输入。期望在x和y方向上的偏移值
        void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
        {
            xoffset *= MouseSensitivity;
            yoffset *= MouseSensitivity;

            Yaw   += xoffset;
            Pitch += yoffset;

            // 确保当俯仰角超出边界时，屏幕不会翻转
            if (constrainPitch)
            {
                if (Pitch > 89.0f)
                    Pitch = 89.0f;
                if (Pitch < -89.0f)
                    Pitch = -89.0f;
            }

            // 使用更新的欧拉角更新前方、右方和上方向量
            updateCameraVectors();
        }

        // 处理从鼠标滚轮事件接收的输入。只需要垂直滚轮轴上的输入
        void ProcessMouseScroll(float yoffset)
        {
            Zoom -= (float)yoffset;
            if (Zoom < 1.0f)
                Zoom = 1.0f;
            if (Zoom > 45.0f)
                Zoom = 45.0f;
        }

        void update(double deltaTime) {
            processInput(deltaTime);
        }

    private:
        //将键盘的输入封装在相机中
        void processInput(double deltaTime) {
            if (Locator::GetService<InputManager>()->GetKey(Key_W) == GLFW_PRESS) {
                ProcessKeyboard(FORWARD, deltaTime);
            }
            if (Locator::GetService<InputManager>()->GetKey(Key_S) == GLFW_PRESS) {
                ProcessKeyboard(BACKWARD, deltaTime);
            }
            if (Locator::GetService<InputManager>()->GetKey(Key_A) == GLFW_PRESS) {
                ProcessKeyboard(LEFT, deltaTime);
            }
            if (Locator::GetService<InputManager>()->GetKey(Key_D) == GLFW_PRESS) {
                ProcessKeyboard(RIGHT, deltaTime);
            }
        }

        // 根据摄像机的（更新的）欧拉角计算前方向量
        void updateCameraVectors()
        {
            // 计算新的前方向量
            glm::vec3 front;
            front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
            front.y = sin(glm::radians(Pitch));
            front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
            Front = glm::normalize(front);

            // 同时重新计算右方和上方向量
            // 标准化向量，因为当你向上或向下看时，它们的长度会接近0，导致移动变慢
            Right = glm::normalize(glm::cross(Front, WorldUp));
            Up    = glm::normalize(glm::cross(Right, Front));
        }
    };
}
#endif