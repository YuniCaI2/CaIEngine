//
// Created by 51092 on 2025/10/14.
//

#ifndef CAIENGINE_SCHEMA_H
#define CAIENGINE_SCHEMA_H
#include<string>
#include<nlohmann/json.hpp>

using GUID = uint64_t;


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

struct BaseMeta {
    GUID id;
    std::string source;
    uint32_t importerVersion; //保证导入器改变时资源解释正确
    AssetType type;
    std::vector<GUID> dependencies{};
    std::string contextHash; //用来校验资源是否改变
};



// Asset

//Texture


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
    TexDim texDim;
    TextureFormat textureFormat {TextureFormat::R8G8B8A8};
    ColorSpace colorSpace {ColorSpace::SRGB};
    bool generateMipmap {true};
    TextureSampler textureSampler{};
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TextureImport, texDim, textureFormat, colorSpace, generateMipmap, textureSampler)


struct TextureAsset {
    GUID id;//和Meta对应
    std::string name;
    uint32_t width;
    uint32_t height;
    uint32_t mipLevels;

    uint32_t numMips;
    uint32_t totalSize;
    std::vector<uint8_t> data;
};

struct TextureMeta : BaseMeta {
    //导入的格式信息
    TextureImport textureImport;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TextureMeta, id, source, importerVersion, type, dependencies, contextHash, textureImport)

struct MaterialAsset {

};

struct MaterialMeta {
    GUID id;

};

struct MeshAsset {

};

struct MeshMeta {

};

struct ShaderAsset {

};

struct ShaderMeta {

};





#endif //CAIENGINE_SCHEMA_H