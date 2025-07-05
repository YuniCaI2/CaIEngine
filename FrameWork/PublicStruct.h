//
// Created by 51092 on 25-6-2.
//

#ifndef PUBLICSTRUCT_H
#define PUBLICSTRUCT_H
#include "pubh.h"
#include "VulkanBuffer.h"
#include "VulkanImage.h"
#include "PublicEnum.h"
#include <vector>


namespace FrameWork {

    struct VulkanFBO {
        std::vector<VkFramebuffer> framebuffers;
        std::vector<uint32_t> AttachmentsIdx; //这里只是使用一个数组存储东西，因为我不确定其中的内容有哪些，所以这里和其对应的renderpass所对应，当然需要检查
        bool inUse = false;
    };

    struct VulkanDescriptor {
        DescriptorType descriptorType; // 将类型暴露出来方便使用
        VkDescriptorPool descriptorPool;
        VkDescriptorSetLayout descriptorSetLayout;
        VkDescriptorSet descriptorSet;
    };

    struct Shader {
        std::vector<VkShaderModule> shaderModules; //shader对应pipeline
        std::vector<VkPipeline> pipelines; //每个pipeline对应一个renderpass
        std::vector<VkRenderPass> renderPasses;
        std::vector<uint32_t> vulkanFBOIdx; //相同每个framebuffer对应一个renderpass
        std::vector<uint32_t> vulkanDescriptorIdx;//其在Vector中的索引就是对应绑定的DescriptorSet的编号
    };

    struct Material {
        std::vector<uint32_t> texturesIdx;
        std::vector<uint32_t> shaderIdx;
    };


    struct Mesh {
        FrameWork::Buffer VertexBuffer;
        FrameWork::Buffer IndexBuffer;
        uint32_t vertexCount;
        uint32_t indexCount;

        bool inUse = false;
    };

    struct Model {
        glm::vec3 position;
        std::vector<Material> materials;
        std::vector<Mesh> meshes;

        bool inUse = false;
    };

    struct TextureFullData {
        int width;
        int height;
        int numChannels;
        unsigned char* data{nullptr};
        std::string path;
        bool isRGB{true};
    };

    struct Texture {
        VulkanImage image;
        VkImageView imageView{VK_NULL_HANDLE};
        VkSampler sampler{VK_NULL_HANDLE}; //optional

        bool inUse = false;
    };


    struct VulkanAttachment {
        std::vector<uint32_t> attachmentsArray; // because flight frame 其中这个uint32_t 的含义是Texture的句柄
        bool inUse = false;
    };

    struct Vertex {
        glm::vec3 position;
        glm::vec3 color;


        //设置描述，以便在创建管线的时期使用
        static auto getBindingDescription() {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Vertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static auto getAttributeDescription() {
            std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);
            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(Vertex, position);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(Vertex, color);

            return attributeDescriptions;
        }
    };

    struct VulkanCommand {
        VkCommandBuffer commandBuffer{VK_NULL_HANDLE};
        std::vector<VkSemaphore> signalSemaphores;
        std::vector<VulkanCommand> commands;
        bool inUse = false;
    };

    struct VulkanDrawCommand {

    };

}

#endif //PUBLICSTRUCT_H
