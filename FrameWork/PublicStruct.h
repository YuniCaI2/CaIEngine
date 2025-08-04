//
// Created by 51092 on 25-6-2.
//

#ifndef PUBLICSTRUCT_H
#define PUBLICSTRUCT_H
#include "pubh.h"
#include "VulkanBuffer.h"
#include "VulkanImage.h"
#include "PublicEnum.h"
#include<optional>
#include <vector>


namespace FrameWork {

    struct VulkanFBO {
        std::vector<VkFramebuffer> framebuffers;
        std::vector<uint32_t> AttachmentsIdx; //这里只是使用一个数组存储东西，因为我不确定其中的内容有哪些，所以这里和其对应的renderpass所对应，当然需要检查
        VkRenderPass renderPass {VK_NULL_HANDLE};
        bool isPresent = false;
        bool isFollowWindow = true;
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
        std::vector<VkDescriptorSet> uniformDescriptorSets;
        std::vector<VkDescriptorSet> textureDescriptorSets;
        std::vector<Buffer> uniformBuffer{};
        std::vector<uint32_t> uniformBufferSizes{};
        std::vector<uint32_t> textures{};

        bool inUse = false;
    };

    struct TextureFullData {
        std::optional<uint32_t> textureID; //兼容纹理创建
        TextureTypeFlagBits type;
        int width;
        int height;
        int numChannels;
        //这里和stb中的对齐
        unsigned char* data{nullptr};
        std::string path;
    };

    struct MaterialCreateInfo {
        std::vector<VkDescriptorSetLayout> UniformDescriptorLayouts;
        std::vector<VkDescriptorSetLayout> TexturesDescriptorLayouts;
        std::vector<std::pair<void*, uint32_t>> UniformData;
                            //data, size
        std::vector<TextureFullData> TexturesDatas;

    };

    struct RayCast {
        glm::vec3 origin;
        glm::vec3 direction;
        int t;
    };

    struct AABB {
        glm::vec3 min{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),std::numeric_limits<float>::max()};
        glm::vec3 max{std::numeric_limits<float>::min(), std::numeric_limits<float>::min(),std::numeric_limits<float>::min()};

        static AABB Union(const AABB& a, const AABB& b) {
            AABB result;
            result.min = glm::min(result.min, a.min);
            result.max = glm::max(result.max, a.max);
            return result;
        }

        glm::vec3 GetCenter() const {
            return (min + max) * 0.5f;
        }

        glm::vec3 Diagonal() const {
            return max - min;
        }

        bool Inside(const glm::vec3& point) const {
            for (int i = 0; i < 3; i++) {
                if (min[i] > point[i] || point[i] > max[i]) {
                    return false;
                }
            }
            return true;
        }

        enum class Extent {
            X,Y,Z
        };

        Extent maxExtent() const { //获取最长边方便BVH区分使用
            auto d = Diagonal();
            if (d.x > d.y && d.x > d.z) {
                return Extent::X;
            }else if (d.y > d.x && d.y > d.z) {
                return Extent::Y;
            }else {
                return Extent::Z;
            }
        }

        bool InterSectingExtent(const RayCast& rayCast) const {
            glm::vec3 inDir = 1.0f / rayCast.direction;

            glm::vec3 tIn = (min - rayCast.origin) * inDir;
            glm::vec3 tOut = (max - rayCast.origin) * inDir;

            glm::vec3 tMin = glm::min(tIn, tOut);
            glm::vec3 tMax = glm::max(tIn, tOut);

            auto tNear = std::max({tMin.x, tMin.y, tMin.z});
            auto tFar = std::min({tMax.x, tMax.y, tMax.z});

            return tNear <= tFar && tFar >= 0;
        }


    };

    struct Mesh {
        FrameWork::Buffer VertexBuffer;
        FrameWork::Buffer IndexBuffer;
        uint32_t vertexCount{0};
        uint32_t indexCount{0};


        //每个网格有对应的渲染优先级来对应不同的渲染程度
        RenderQueue renderQueue{RenderQueue::Opaque};

        bool inUse = false;
    };

    struct Model {
        glm::vec3 position;
        //材质和网格是一一对应的
        std::vector<uint32_t> materials;
        std::vector<uint32_t> meshes;
        using TriangleBoundingBoxPtr = std::unique_ptr<std::vector<AABB>>;
        AABB aabb;
        TriangleBoundingBoxPtr triangleBoundingBoxs;

        bool inUse = false;
    };


    struct Texture {
        VulkanImage image;
        VkImageView imageView{VK_NULL_HANDLE};
        VkSampler sampler{VK_NULL_HANDLE}; //optional

        TextureTypeFlagBits textureType;//这个先没使用

        bool inUse = false;
    };


    struct VulkanAttachment {
        std::vector<uint32_t> attachmentsArray; // because flight frame 其中这个uint32_t 的含义是Texture的句柄
        AttachmentType type;
        uint32_t width;
        uint32_t height;
        VkSampleCountFlagBits samples{VK_SAMPLE_COUNT_1_BIT};
        bool isSampled = false;
        bool inUse = false;
    };

    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec3 tangent;
        glm::vec2 texCoord;


        //设置描述，以便在创建管线的时期使用
        static auto getBindingDescription() {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Vertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static auto getAttributeDescription() {
            std::vector<VkVertexInputAttributeDescription> attributeDescriptions(4);
            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(Vertex, position);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(Vertex, normal);


            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[2].offset = offsetof(Vertex, tangent);

            attributeDescriptions[3].binding = 0;
            attributeDescriptions[3].location = 3;
            attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[3].offset = offsetof(Vertex, texCoord);

            return attributeDescriptions;
        }
    };

    struct MeshData {
        std::vector<uint32_t> indices;
        std::vector<Vertex> vertices;
        std::vector<TextureFullData> texData;
    };

    struct ModelData {
        std::vector<MeshData> meshDatas;
    };

}

#endif //PUBLICSTRUCT_H
