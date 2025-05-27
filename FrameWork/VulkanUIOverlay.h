//
// Created by 51092 on 25-5-10.
//

#ifndef VULKANUIOVERLAY_H
#define VULKANUIOVERLAY_H
#include <glm/vec2.hpp>

#include "VulkanDevice.h"
#include "pubh.h"

#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "VulkanSwapChain.h"

// 在类声明前添加版本检查
#if IMGUI_VERSION_NUM < 18700
#error "ImGui version too old. Please use ImGui 1.87 or newer."
#endif


namespace FrameWork {
    class VulkanUIOverlay {
    public:
        FrameWork::VulkanDevice *device;

        VkSampleCountFlagBits rasterizationSamples{VK_SAMPLE_COUNT_1_BIT};

        VkDescriptorPool descriptorPool{VK_NULL_HANDLE};


        struct PushConstBlock {
            glm::vec2 scale;
            glm::vec2 translate;
        } pushConstBlock;

        bool visible{true};
        bool updated{true};
        float scale{1.0f};
        float updateTimer{0.0f};

        VulkanUIOverlay();

        ~VulkanUIOverlay();

        void prepareResources(GLFWwindow* window,VkInstance instance,
             VkRenderPass &renderPass,  VulkanSwapChain& swapChain, VkQueue queue);


        void draw(const VkCommandBuffer &commandBuffer);

        void resize(uint32_t width, uint32_t height);

        void freeResources();

        bool header(const char *caption);

        bool checkBox(const char *caption, bool *value);

        bool checkBox(const char *caption, int32_t *value);

        bool radioButton(const char *caption, bool value);

        bool inputFloat(const char *caption, float *value, float step, uint32_t precision);

        bool sliderFloat(const char *caption, float *value, float min, float max);

        bool sliderInt(const char *caption, int32_t *value, int32_t min, int32_t max);

        bool comboBox(const char *caption, int32_t *itemindex, std::vector<std::string> items);

        bool button(const char *caption);

        bool colorPicker(const char *caption, float *color);

        void text(const char *formatstr, ...);
    };
}


#endif //VULKANUIOVERLAY_H
