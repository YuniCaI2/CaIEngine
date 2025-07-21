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

    struct VulkanPipelineInfo {
        std::unordered_map<VkShaderStageFlagBits, VkPipelineShaderStageCreateInfo> shaderModules;
        //顶点输入状况
        std::vector<VkVertexInputBindingDescription> vertexBindingDescriptions;
        std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions;

        VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE //禁用片元重启
            //片元重启：可以分离一组三角形条带为两组，进而减少调用DrawCall的数量
        };

        //视口状态
        VkViewport viewport;
        VkRect2D scissor;

        VkPipelineViewportStateCreateInfo viewportState;
        VkPipelineRasterizationStateCreateInfo rasterizationState;
        VkPipelineMultisampleStateCreateInfo multisampleState;
        VkPipelineDepthStencilStateCreateInfo depthStencilState;
        std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachmentStates;

        //管线默认接受动态更新视口大小和和裁剪窗口大小

        bool inUse = false;
    };

    struct VulkanPipeline{
        VkPipeline pipeline {VK_NULL_HANDLE};
        VkPipelineLayout pipelineLayout {VK_NULL_HANDLE};
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
        uint32_t pipelineInfoIdx = -1;

        bool inUse = false;
    };

    struct Material {
        //此处规定texture的DescriptorSet在Uniform后部
        std::vector<std::pair<VkDescriptorSet,VkDescriptorSetLayout>> descriptorPairs{};
        std::vector<VkDescriptorSet> descriptorSets;
        std::vector<Buffer> uniformBuffer{};
        std::vector<uint32_t> uniformBufferSizes{};
        std::vector<uint32_t> textures{};

        bool inUse = false;
    };

    struct TextureFullData {
        std::optional<uint32_t> textureID; //兼容纹理创建

        int width;
        int height;
        int numChannels;
        unsigned char* data{nullptr};
        std::string path;
        bool isRGB{true};

    };

    struct MaterialCreateInfo {
        std::vector<VkDescriptorSetLayout> UniformDescriptorLayouts;
        std::vector<VkDescriptorSetLayout> TexturesDescriptorLayouts;
        std::vector<std::pair<void*, uint32_t>> UniformData;
                            //data, size
        std::vector<TextureFullData> TexturesDatas;

    };

    struct Mesh {
        FrameWork::Buffer VertexBuffer;
        FrameWork::Buffer IndexBuffer;
        uint32_t vertexCount;
        uint32_t indexCount;

        //每个网格有对应的渲染优先级来对应不同的渲染程度
        RenderQueue renderQueue{RenderQueue::Opaque};

        bool inUse = false;
    };

    struct Model {
        glm::vec3 position;
        //材质和网格是一一对应的
        std::vector<Material> materials;
        std::vector<Mesh> meshes;

        bool inUse = false;
    };


    struct Texture {
        VulkanImage image;
        VkImageView imageView{VK_NULL_HANDLE};
        VkSampler sampler{VK_NULL_HANDLE}; //optional

        bool inUse = false;
    };


    struct VulkanAttachment {
        std::vector<uint32_t> attachmentsArray; // because flight frame 其中这个uint32_t 的含义是Texture的句柄
        AttachmentType type;
        uint32_t width;
        uint32_t height;
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

}

#endif //PUBLICSTRUCT_H
