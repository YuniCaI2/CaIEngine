//
// Created by 51092 on 25-5-10.
//

#ifndef VULKANUIOVERLAY_H
#define VULKANUIOVERLAY_H
#include <glm/vec2.hpp>

#include "VulkanDevice.h"
#include <imgui.h>
#include "pubh.h"


namespace FrameWork {
    class VulkanUIOverlay {
    public:
        FrameWork::VulkanDevice *device;
        VkQueue queue{VK_NULL_HANDLE};

        VkSampleCountFlagBits rasterizationSamples{VK_SAMPLE_COUNT_1_BIT};
        uint32_t subpass{0};

        Buffer vertexBuffer;
        Buffer indexBuffer;
        int32_t vertexCount{0};
        int32_t indexCount{0};

        std::vector<VkPipelineShaderStageCreateInfo> shaders;

        VkDescriptorPool descriptorPool{VK_NULL_HANDLE};
        VkDescriptorSetLayout descriptorSetLayout{VK_NULL_HANDLE};
        VkDescriptorSet descriptorSet{VK_NULL_HANDLE};
        VkPipelineLayout pipelineLayout{VK_NULL_HANDLE};
        VkPipeline pipeline{VK_NULL_HANDLE};

        VkDeviceMemory fontMemory{VK_NULL_HANDLE};
        VkImage fontImage{VK_NULL_HANDLE};
        VkImageView fontView{VK_NULL_HANDLE};
        VkSampler sampler{VK_NULL_HANDLE};

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

        void preparePipeline(const VkPipelineCache &pipelineCache, const VkRenderPass &renderPass,
                             const VkFormat &colorFormat, const VkFormat &depthFormat);

        void prepareResources();

        bool update();

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
