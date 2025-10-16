//
// Created by 51092 on 2025/10/14.
//

#ifndef CAIENGINE_SCHEMA_H
#define CAIENGINE_SCHEMA_H
#include <cstdint>
#include<string>
#include<nlohmann/json.hpp>
#include<glm/glm.hpp>
#include"PublicStruct.h"

enum class SchemaType {
    Asset,
    Component,
    Meta,
    Scene,
    Editor
};

enum class AssetType {
    Texture,
    Material,
    Mesh,
    Shader
};
NLOHMANN_JSON_SERIALIZE_ENUM(AssetType,
    {
        {AssetType::Texture, "Texture"},
        {AssetType::Material, "Material"},
        {AssetType::Mesh, "Mesh"},
        {AssetType::Shader, "Shader"}
    }
    )


enum class TextureFormat {
    R8,
    R8G8B8A8,
    R16G16B16
};
enum class ColorSpace {
    LINEAR,
    SRGB
};

//支持的纹理
enum class TexDim {
    Tex2D,
    Tex3D,
    Cube
};

enum class SamplerFilter {
    Nearest,
    Linear,
};

enum class SamplerWrap {
    Repeat,
    ClampToEdge
};



NLOHMANN_JSON_SERIALIZE_ENUM(TextureFormat,
    {
        {TextureFormat::R8, "R8"},
        {TextureFormat::R8G8B8A8, "R8G8B8A8"},
        {TextureFormat::R16G16B16, "R16G16B16"}
    }
    )
NLOHMANN_JSON_SERIALIZE_ENUM(ColorSpace,
    {
        {ColorSpace::LINEAR, "Linear"},
        {ColorSpace::SRGB, "SRGB"}
    }
    )
NLOHMANN_JSON_SERIALIZE_ENUM(SamplerFilter,
    {
        {SamplerFilter::Nearest, "nearest"},
        {SamplerFilter::Linear, "linear"}
    }
    )

NLOHMANN_JSON_SERIALIZE_ENUM(SamplerWrap,
    {
        {SamplerWrap::Repeat, "Repeat"},
        {SamplerWrap::ClampToEdge, "ClampToEdge"}
    }
    )

NLOHMANN_JSON_SERIALIZE_ENUM(TexDim,
    {
        {TexDim::Tex2D, "Tex2D"},
        {TexDim::Tex3D, "Tex3D"},
        {TexDim::Cube, "Cube"}
    }
    )

struct TextureSampler {
    SamplerFilter minFilter{SamplerFilter::Linear};
    SamplerFilter maxFilter{SamplerFilter::Linear};
    SamplerWrap wrap{SamplerWrap::ClampToEdge};
    uint32_t anisotropy{};
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TextureSampler, minFilter, maxFilter, wrap, anisotropy)

struct TextureImport {
    TexDim texDim{TexDim::Tex2D};
    TextureFormat textureFormat {TextureFormat::R8G8B8A8};
    ColorSpace colorSpace {ColorSpace::SRGB};
    bool generateMipmap {true};
    TextureSampler textureSampler{};
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TextureImport, texDim, textureFormat, colorSpace, generateMipmap, textureSampler)


struct TextureAsset {
    std::string name;
    std::string sourcePath;
    uint32_t width;
    uint32_t height;
    uint32_t numChannel;
    TextureImport textureImport;

    unsigned char* data;
};


struct MaterialProperties {
    template<class T>
    using ParamMap = std::map<std::string, T>;

    //Uniform
    ParamMap<float> floatParams{};
    ParamMap<uint32_t> uintParams{};
    ParamMap<int> intParams{};
    ParamMap<glm::vec2> vec2Params{};
    ParamMap<glm::vec3> vec3Params{};
    ParamMap<glm::vec4> vec4Params{};
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MaterialProperties,
    floatParams, uintParams, intParams, vec2Params, vec3Params, vec4Params
)

struct MaterialAsset {
    std::string name;
    std::string shaderPath;
    MaterialProperties properties;
};

struct VertexData {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec2 texCoord;
};

struct MeshAsset { //理应上Material和Mesh不耦合
    std::string name;
    std::vector<VertexData> vertices;
    std::vector<uint32_t> indices;
};


struct ShaderPass {
    std::string passName;
    std::string sourcePath;
    FrameWork::ShaderInfo shaderInfo;
    uint32_t shaderSize;

    uint32_t* shaderCode; //shaderCode //存储成bin文件
};


struct ShaderRenderGraph {

};



struct ShaderAsset {
    std::string name;
    std::vector<std::unique_ptr<ShaderPass>> passes;

};


struct ModelImport {
    bool genNormal{true};
    bool genTangent{true};
    bool filpUV{false};
    float scale{1.0f};
    //...
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ModelImport, genNormal, genTangent, filpUV, scale);

struct ModelNode {
    uint32_t index; //在Model中Nodes array中的索引
    uint32_t parentIndex{UINT32_MAX};
    std::vector<uint32_t> childrenIndices{};

    std::string name;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ModelNode, index, parentIndex, childrenIndices, name)

struct ModelAsset {
    ModelImport import;
    std::string name;
    uint32_t rootNode;
    std::vector<ModelNode> nodes;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ModelAsset, name, rootNode, nodes);

//Comp Material
//Comp Shader
//后续需要补充Comp Shader Comp Material， 但是CompMaterial的传入的参数是transient，则不需要Material Properties 设置
// Static Param。如果传入的参数是Static，那么大概率计算管线不需要每帧执行。



#endif //CAIENGINE_SCHEMA_H
