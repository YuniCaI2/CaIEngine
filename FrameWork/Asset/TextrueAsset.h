//
// Created by 51092 on 2025/10/16.
//

#ifndef CAIENGINE_TEXTRUEASSET_H
#define CAIENGINE_TEXTRUEASSET_H
#include<nlohmann/json.hpp>

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


#endif //CAIENGINE_TEXTRUEASSET_H