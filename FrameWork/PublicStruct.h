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
#include "Light.h"
#include <unordered_map>


namespace FrameWork {

    struct VulkanFBO {
        std::vector<VkFramebuffer> framebuffers;
        std::vector<uint32_t> AttachmentsIdx; //这里只是使用一个数组存储东西，因为我不确定其中的内容有哪些，所以这里和其对应的renderpass所对应，当然需要检查
        VkRenderPass renderPass {VK_NULL_HANDLE};
        bool isPresent = false;
        bool isFollowWindow = true;
        bool inUse = false;
        uint32_t width {0};
        uint32_t height {0};
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

        bool inUse = false;
    };


    struct Material {
        //此处规定texture的DescriptorSet在Uniform后部
        std::vector<uint32_t> textures{};
        bool inUse = false;
    };

    //套壳保证和texture对于slot的接口一致性
    struct StorageBuffer {
        Buffer buffer{};
        uint32_t itemNum{};

        bool inUse = false;
    };

    struct VulkanShader {
        uint32_t pipelineID = -1;
        uint32_t MaterialID = -1;
        std::vector<uint32_t> uniformBufferIDs;
        std::vector<StorageBuffer> storageBufferIDs;
        VkRenderPass renderPass {VK_NULL_HANDLE};

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

        int InterSectingExtent(const RayCast& rayCast) const {
            glm::vec3 inDir = 1.0f / rayCast.direction;

            glm::vec3 tIn = (min - rayCast.origin) * inDir;
            glm::vec3 tOut = (max - rayCast.origin) * inDir;

            glm::vec3 tMin = glm::min(tIn, tOut);
            glm::vec3 tMax = glm::max(tIn, tOut);

            auto tNear = std::max({tMin.x, tMin.y, tMin.z});
            auto tFar = std::min({tMax.x, tMax.y, tMax.z});

            if (tNear <= tFar && tFar > 0) {
                if (tNear < 0) {
                    return tFar;
                }else {
                    return tNear;
                }
            }else {
                return -1;
            }
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
        std::vector<uint32_t> materialSlots;
        std::vector<uint32_t> materials;
        std::vector<uint32_t> meshes;
        using TriangleBoundingBoxPtr = std::unique_ptr<std::vector<AABB>>;
        AABB aabb;
        TriangleBoundingBoxPtr triangleBoundingBoxs;

        bool inUse = false;
    };



    struct Texture {
        VulkanImage image{};
        VkImageView imageView{VK_NULL_HANDLE};
        VkSampler sampler{VK_NULL_HANDLE}; //optional
        bool isSwapChainRef{false}; //这里用来适配FrameGraph的资源导入
        bool inUse = false;
    };


    struct VulkanAttachment {
        std::vector<uint32_t> attachmentsArray; // because flight frame 其中这个uint32_t 的含义是Texture的句柄
        AttachmentType type;
        uint32_t width;
        uint32_t height;
        VkSampleCountFlagBits samples{VK_SAMPLE_COUNT_1_BIT};
        bool isSampled = false;
        bool isFollowWindow = true;
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

    //这个ModelData的Vulkan资源话，
    //尽可能的浅封装,不将我的Material封装进去
    //现在的model还是static
    struct VulkanModelData {
        using TextureMap = std::unordered_map<TextureTypeFlagBits, uint32_t>;
        std::vector<TextureMap> textures;
        std::vector<uint32_t> meshIDs{}; //网格ID
        glm::vec3 position{0,0,0}; //初始位置
        using TriangleBoundingBoxPtr = std::unique_ptr<std::vector<AABB>>;
        AABB aabb;
        TriangleBoundingBoxPtr triangleBoundingBoxs;
        bool inUse = false;
    };

    struct RenderObject {
        uint32_t meshID{};
        uint32_t slotID{};
    };


    struct ShaderStateSet {
        BlendOption blendOp = BlendOption::ADD;
        BlendFactor srcBlendFactor = BlendFactor::SRC_ALPHA;
        BlendFactor dstBlendFactor = BlendFactor::ONE_MINUS_SRC_ALPHA;
        FaceCullOption faceCullOp = FaceCullOption::Back;
        CompareOption depthCompareOp = CompareOption::LESS; //深度测试相关
        PolygonMode polygonMode = PolygonMode::Fill;//这里为了区分渲染是直线还是填充

        //渲染队列，这里先不加
        bool depthWrite = true;
        bool inputVertex = true;
        bool msaa = false;

        //输出附件的数量
        uint32_t outputNums{};
    };

    struct ShaderProperty {
        std::string name {};
        uint32_t size = 0;        // 整个属性大小(如果是数组则代表整个数组的大小)
        uint32_t align = 0;       // 单个属性的对齐标准
        uint32_t offset = 0;      // 属性在Uniform Buffer(Vulkan)或Constant Buffer(D3D12)中的偏移量
        uint32_t binding = 0;     // 在Vulkan中代表Uniform Buffer和纹理的layout binding
        uint32_t arrayLength = 0; // 属性数组长度
        uint32_t arrayOffset = 0; // 如果是数组的话，一个属性在数组内的偏移量
        ShaderPropertyType type = ShaderPropertyType::FLOAT;
    };

    struct ShaderPropertiesInfo
    {
        std::vector<ShaderProperty> baseProperties;
        std::vector<ShaderProperty> textureProperties;
    };

    struct ShaderInfo {
        ShaderStateSet shaderState; //基本的状态设置
        ShaderTypeFlags shaderTypeFlags = 0;
        ShaderPropertiesInfo vertProperties;
        ShaderPropertiesInfo fragProperties;
        //这里先省略几何着色器，后面在加
    };

    struct CompLocalInvocation {
        uint32_t x = 1;
        uint32_t y = 1;
        uint32_t z = 1;
    };

    struct SSBO {
        std::string name{};
        std::string structName{}; //存储StorageBuffer的Struct
        StorageObjectType type{};
        SSBO_OP ssboOP;
        uint32_t binding = 0;
    };


    struct CompShaderInfo {
        CompLocalInvocation localInvocation;
        ShaderPropertiesInfo shaderProperties;
        std::vector<SSBO> ssbos;
    };


    struct MaterialData { //用来存储Vulkan中的资源
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts; //此处仅仅只是一个引用不管理其生命周期
        std::vector<VkDescriptorSet> descriptorSets;
        std::vector<Buffer> vertexUniformBuffers; //和descriptorSet对应
        std::vector<Buffer> fragmentUniformBuffers;
        //等到需要使用几何着色器的时候进行添加

        bool inUse = false;
    };



    struct CompMaterialData {
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
        std::vector<VkDescriptorSet> descriptorSets;
        std::vector<Buffer> uniformBuffers;

        bool inUse = false;
    };


}


#endif //PUBLICSTRUCT_H
