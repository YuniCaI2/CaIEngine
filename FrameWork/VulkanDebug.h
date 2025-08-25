//
// Created by 51092 on 25-5-22.
//

#ifndef VULKANDEBUG_H
#define VULKANDEBUG_H

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include<unordered_map>

#include "PublicStruct.h"
#include "Slot.h"

namespace FrameWork {
    class VulkanDebug {
    public:
        static VkDebugUtilsMessengerEXT debugUtilsMessenger;

        // 函数指针声明
        static PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
        static PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;

        static VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessageCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
            void *pUserData);

        static void setDebugging(VkInstance instance);
        static void freeDebugCallBack(VkInstance instance);
        static void setupDebuggingMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &debug);
    };

    namespace debugUtils {
        // 函数指针声明
        extern PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT;
        extern PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT;

        void setup(VkInstance instance);
        void cmdBeginLabel(VkCommandBuffer cmdBuffer, std::string caption, glm::vec4 color);
        void cmdEndLabel(VkCommandBuffer cmdBuffer);
    }

    class AABBDeBugging {
    public:
        void Init(const std::string& shaderName, uint32_t colorAttachmentID);//确定ubo的格式
        void GenerateAABB(uint32_t modelID);
        void Draw(VkCommandBuffer cmdBuffer);
        void Update(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);
        void Destroy();


    private:
        uint32_t debugPipelineID = -1;
        uint32_t frameBufferID = -1;
        VkPipeline debugPipeline {VK_NULL_HANDLE};
        VkPipelineLayout debugPipelineLayout {VK_NULL_HANDLE};
        VkRenderPass debugRenderPass{VK_NULL_HANDLE};
        VkDescriptorSetLayout uniformDescriptorSetLayout{VK_NULL_HANDLE};
        std::unordered_map<uint32_t , Slot> slots;
        glm::mat4 viewMatrix{};
        glm::mat4 projectionMatrix{};
        glm::mat4 modelMatrix{};

        //以model为单位
        std::unordered_map<uint32_t, Buffer> vertexBuffers{};
        std::unordered_map<uint32_t, Buffer> indexBuffers{};
        std::unordered_map<uint32_t, uint32_t> materialIds{};
        std::unordered_map<uint32_t, uint32_t> indicesCounts{};

        struct LineVertex {
            glm::vec3 position;
            glm::vec3 color;

            static auto getBindingDescription() {
                VkVertexInputBindingDescription bindingDescription{};
                bindingDescription.binding = 0;
                bindingDescription.stride = sizeof(LineVertex);
                bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

                return bindingDescription;
            }

            static auto getAttributeDescription() {
                std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);
                attributeDescriptions[0].binding = 0;
                attributeDescriptions[0].location = 0;
                attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
                attributeDescriptions[0].offset = offsetof(LineVertex, position);

                attributeDescriptions[1].binding = 0;
                attributeDescriptions[1].location = 1;
                attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
                attributeDescriptions[1].offset = offsetof(LineVertex, color);

                return attributeDescriptions;
            }

        };

        struct UniformBufferObject {
            glm::mat4 model;
            glm::mat4 view;
            glm::mat4 proj;

            void Update(const glm::mat4* viewMatrix, const glm::mat4* projection, const glm::vec3* position) {
                view = *viewMatrix;
                proj = *projection;
                this->model = glm::translate(glm::mat4(1.0f), *position);
            }

        };

        UniformBufferObject ubo{};

        std::vector<LineVertex> lines;
        std::vector<uint32_t> indices;

        std::vector<LineVertex> GenerateLineVertex(const AABB& aabb, glm::vec3&& color);
        std::vector<uint32_t> GenerateLineIndices(uint32_t baseID);
    };
}

#endif //VULKANDEBUG_H