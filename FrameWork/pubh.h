#ifndef PUBH_H
#define PUBH_H



#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cassert>  // 添加这个，因为您用了 assert



// GLM 定义
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // Vulkan 通常使用这个深度范围
#define GLM_ENABLE_EXPERIMENTAL     // 如果你使用了 GLM 的实验性特性
// GLM 包含
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp> // 如果需要
#include <glm/gtc/type_ptr.hpp>       // 如果需要

//Assimp


//一些别名


#endif //PUBH_H