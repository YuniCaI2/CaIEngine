//
// Created by 51092 on 25-8-28.
//

#ifndef SHADERPARSE_H
#define SHADERPARSE_H
#include "PublicStruct.h"
#include <unordered_map>

namespace FrameWork {
    struct PropertyAlignInfo {
        uint32_t size = 0; //尺寸
        uint32_t alignment = 0; //对齐单位，如果是数组则是数组内的对齐单位，MAT可看成数组的数组
        uint32_t arrayOffset = 0; //将数组中一个对象的对齐单位
        //根据std140在数组中最少要16字节对齐
    };

    class ShaderParse {
    public:
        static ShaderInfo GetShaderInfo(const std::string &code);

        static void ParseShaderCode(const std::string &code, std::string &vert, std::string &frag); //后续在处理几何着色器和计算着色器
        static ShaderPropertiesInfo GetShaderProperties(const std::string &code);
        static bool IsBaseProperty(ShaderPropertyType type);
        static std::string TranslateToVulkan(const std::string &code, const ShaderPropertiesInfo& properties);

        //Comp
        static CompShaderInfo GetCompShaderInfo(const std::string &code);
        static std::string TranslateCompToVulkan(const std::string &code, const CompShaderInfo& compShaderInfo);

        // private:
        // 为了测试
        static ShaderStateSet GetShaderStateSet(const std::string &code);
        static CompLocalInvocation GetCompLocalInvocation(const std::string& code);
        static std::vector<SSBO> GetSSBOs(const std::string &code);
        static void SetUpCompSSBOAndProperty(CompShaderInfo& info);

        static std::string GetCodeBlock(const std::string &code, const std::string &blockName);
        static void SetUpPropertiesStd140(ShaderInfo &shaderInfo);
        static PropertyAlignInfo GetPropertyAlignInfoStd140(ShaderPropertyType type, uint32_t arrayLength);
        static bool isValidChar(const char &c);
        static size_t FindWord(const std::string &code, const std::string &word, size_t offset);
        static void RemoveCRLF(std::string &code);
        static void RemoveSpaces(std::string &code);
        static std::vector<std::string> SplitString(const std::string &str, char p);
        static std::vector<std::string> ExtractWords(const std::string &str);
        static void GetPropertyNameAndArrayLength(const std::string &propertyStr, std::string &name,
                                                  uint32_t &arrayLength);
        static void ReplaceAllWordsInBlock(std::string& blockCode, const std::string& src, const std::string& dst);



        //Map
        inline static std::unordered_map<std::string, ShaderPropertyType> shaderPropertyMap = {
            {"vec2", ShaderPropertyType::VEC2}, {"vec3", ShaderPropertyType::VEC3}, {"vec4", ShaderPropertyType::VEC4},
            {"ivec2", ShaderPropertyType::IVEC2}, {"ivec3", ShaderPropertyType::IVEC3},
            {"ivec4", ShaderPropertyType::IVEC4},
            {"uvec2", ShaderPropertyType::UVEC2}, {"uvec3", ShaderPropertyType::UVEC3},
            {"uvec4", ShaderPropertyType::UVEC4},
            {"mat2", ShaderPropertyType::MAT2}, {"mat3", ShaderPropertyType::MAT3}, {"mat4", ShaderPropertyType::MAT4},
            {"int", ShaderPropertyType::INT}, {"uint", ShaderPropertyType::UINT}, {"float", ShaderPropertyType::FLOAT},
            {"bool", ShaderPropertyType::BOOL},
            {"sampler", ShaderPropertyType::SAMPLER}, {"sampler2D", ShaderPropertyType::SAMPLER_2D},
            {"samplerCube", ShaderPropertyType::SAMPLER_CUBE}
        };

        inline static std::unordered_map<std::string, BlendOption> blendOptionMap = {
            {"Add", BlendOption::ADD}, {"Sub", BlendOption::SUBTRACT}, {"RevSub", BlendOption::REVERSE_SUBTRACT},
            {"Min", BlendOption::MIN}, {"Max", BlendOption::MAX}
        };

        inline static std::unordered_map<std::string, FaceCullOption> faceCullOptionMap = {
            {"Back", FaceCullOption::Back}, {"Front", FaceCullOption::Front},
            {"None", FaceCullOption::None}, {"All", FaceCullOption::FrontAndBack}
        };

        inline static std::unordered_map<std::string, CompareOption> depthTestOptionMap = {
            {"Never", CompareOption::NEVER}, {"Less", CompareOption::LESS},
            {"LessOrEqual", CompareOption::LESS_OR_EQUAL},
            {"Always", CompareOption::ALWAYS}, {"Greater", CompareOption::GREATER},
            {"GreaterOrEqual", CompareOption::GREATER_OR_EQUAL},
            {"Equal", CompareOption::EQUAL}, {"NotEqual", CompareOption::NOT_EQUAL},
        };

        inline static std::unordered_map<ShaderPropertyType, std::string> propertyTypeMapToGLSL = {
            {ShaderPropertyType::VEC2, "vec2"}, {ShaderPropertyType::VEC3, "vec3"}, {ShaderPropertyType::VEC4, "vec4"},
            {ShaderPropertyType::IVEC2, "ivec2"}, {ShaderPropertyType::IVEC3, "ivec3"},
            {ShaderPropertyType::IVEC4, "ivec4"},
            {ShaderPropertyType::UVEC2, "uvec2"}, {ShaderPropertyType::UVEC3, "uvec3"},
            {ShaderPropertyType::UVEC4, "uvec4"},
            {ShaderPropertyType::MAT2, "mat2"}, {ShaderPropertyType::MAT3, "mat3"}, {ShaderPropertyType::MAT4, "mat4"},
            {ShaderPropertyType::INT, "int"}, {ShaderPropertyType::UINT, "uint"}, {ShaderPropertyType::FLOAT, "float"},
            {ShaderPropertyType::BOOL, "bool"},
        };

        inline static std::unordered_map<std::string, BlendFactor> blendFactorMap =
        {
            {"Zero", BlendFactor::ZERO}, {"One", BlendFactor::ONE},
            {"SrcColor", BlendFactor::SRC_COLOR}, {"OneMinusSrcColor", BlendFactor::ONE_MINUS_SRC_COLOR},
            {"DstColor", BlendFactor::DST_COLOR}, {"OneMinusDstColor", BlendFactor::ONE_MINUS_DST_COLOR},
            {"SrcAlpha", BlendFactor::SRC_ALPHA}, {"OneMinusSrcAlpha", BlendFactor::ONE_MINUS_SRC_ALPHA},
            {"DstAlpha", BlendFactor::DST_ALPHA}, {"OneMinusDstAlpha", BlendFactor::ONE_MINUS_DST_ALPHA},
            {"ConstantColor", BlendFactor::CONSTANT_COLOR},
            {"OneMinusConstantColor", BlendFactor::ONE_MINUS_CONSTANT_COLOR},
            {"ConstantAlpha", BlendFactor::CONSTANT_ALPHA},
            {"OneMinusConstantAlpha", BlendFactor::ONE_MINUS_CONSTANT_ALPHA},
        };


        inline static std::unordered_map<std::string, PolygonMode> polygonModeMap = {
            {"Line", PolygonMode::Line},
            {"Fill", PolygonMode::Fill},
        };

        inline static std::unordered_map<std::string, SSBO_OP> ssboOpMap = {
            {"Write", SSBO_OP::Write},
            {"Read", SSBO_OP::Read},
            {"WriteRead", SSBO_OP::WriteRead},
        };

        inline static std::unordered_map<std::string, StorageObjectType> storageObjectTypeMap = {
            {"Image2D", StorageObjectType::Image2D},
            {"Image3D", StorageObjectType::Image3D},
            {"ImageCube", StorageObjectType::ImageCube},
            {"Buffer", StorageObjectType::Buffer},
        };

        inline static std::unordered_map<std::string, StorageImageFormat> storageImageFormatMap = {
            {"RGBA8", StorageImageFormat::RGBA8},
            {"RGBA16F", StorageImageFormat::RGBA16F},
        };

        inline static std::unordered_map<BlendOption, VkBlendOp> blendOpToVulkanBlendOp = {
            {BlendOption::ADD, VK_BLEND_OP_ADD}, {BlendOption::MAX, VK_BLEND_OP_MAX},
            {BlendOption::MIN, VK_BLEND_OP_MIN}, {BlendOption::SUBTRACT, VK_BLEND_OP_SUBTRACT},
            {BlendOption::REVERSE_SUBTRACT, VK_BLEND_OP_REVERSE_SUBTRACT}
        };

        inline static std::unordered_map<BlendFactor, VkBlendFactor> blendFactorToVulkanBlendFactor = {
            {BlendFactor::ZERO, VK_BLEND_FACTOR_ZERO},
            {BlendFactor::ONE, VK_BLEND_FACTOR_ONE},
            {BlendFactor::SRC_COLOR, VK_BLEND_FACTOR_SRC_COLOR},
            {BlendFactor::ONE_MINUS_SRC_COLOR, VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR},
            {BlendFactor::DST_COLOR, VK_BLEND_FACTOR_DST_COLOR},
            {BlendFactor::ONE_MINUS_DST_COLOR, VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR},
            {BlendFactor::SRC_ALPHA, VK_BLEND_FACTOR_SRC_ALPHA},
            {BlendFactor::ONE_MINUS_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA},
            {BlendFactor::DST_ALPHA, VK_BLEND_FACTOR_DST_ALPHA},
            {BlendFactor::ONE_MINUS_DST_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA},
            {BlendFactor::CONSTANT_COLOR, VK_BLEND_FACTOR_CONSTANT_COLOR},
            {BlendFactor::ONE_MINUS_CONSTANT_COLOR, VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR},
            {BlendFactor::CONSTANT_ALPHA, VK_BLEND_FACTOR_CONSTANT_ALPHA},
            {BlendFactor::ONE_MINUS_CONSTANT_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA}
        };

        inline static std::unordered_map<PolygonMode, VkPolygonMode> polygonModeToVulkanPolygonMode = {
           {PolygonMode::Line, VK_POLYGON_MODE_LINE},
            {PolygonMode::Fill, VK_POLYGON_MODE_FILL},
        };

        inline static std::unordered_map<FaceCullOption, VkCullModeFlags> cullModeToVulkanCullMode = {
            {FaceCullOption::Front, VK_CULL_MODE_FRONT_BIT},
            {FaceCullOption::Back, VK_CULL_MODE_BACK_BIT},
            {FaceCullOption::None, VK_CULL_MODE_NONE},
            {FaceCullOption::FrontAndBack, VK_CULL_MODE_FRONT_AND_BACK}
        };

        inline static std::unordered_map<CompareOption, VkCompareOp> compareModeToVulkanCompareMode = {
            {CompareOption::LESS, VK_COMPARE_OP_LESS},
            {CompareOption::EQUAL, VK_COMPARE_OP_EQUAL},
            {CompareOption::NEVER, VK_COMPARE_OP_NEVER},
            {CompareOption::ALWAYS, VK_COMPARE_OP_ALWAYS},
            {CompareOption::GREATER, VK_COMPARE_OP_GREATER},
            {CompareOption::NOT_EQUAL, VK_COMPARE_OP_NOT_EQUAL},
            {CompareOption::GREATER_OR_EQUAL, VK_COMPARE_OP_GREATER_OR_EQUAL},
            {CompareOption::LESS_OR_EQUAL, VK_COMPARE_OP_LESS_OR_EQUAL},
        };

        inline static std::unordered_map<SSBO_OP, std::string> ssboOpToString = {
            {SSBO_OP::Write, "writeonly"},
            {SSBO_OP::Read, "readonly"},
            {SSBO_OP::WriteRead, ""}
        };

        inline static std::unordered_map<StorageObjectType, std::string> storageTypeToStringType = {
            {StorageObjectType::Image2D, "image2D"},
            {StorageObjectType::Image3D, "image3D"},
            {StorageObjectType::ImageCube, "imageCube"},
            {StorageObjectType::Buffer, "buffer"}
        };

        inline static std::unordered_map<StorageObjectType, VkDescriptorType> storageTypeToDescriptorType = {
            {StorageObjectType::Image2D, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE},
            {StorageObjectType::Image3D, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE},
            {StorageObjectType::ImageCube, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE},
            {StorageObjectType::Buffer, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER}
        };

        inline static std::unordered_map<StorageImageFormat, std::string> storageImageFormatToString = {
            {StorageImageFormat::RGBA8, "rgba8"},
            {StorageImageFormat::RGBA16F, "rgba16f"},
        };

    };
}


#endif //SHADERPARSE_H
