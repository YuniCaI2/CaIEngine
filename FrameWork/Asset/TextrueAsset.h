//
// Created by 51092 on 2025/10/16.
//

#ifndef CAIENGINE_TEXTRUEASSET_H
#define CAIENGINE_TEXTRUEASSET_H
#include<nlohmann/json.hpp>
#include<fstream>

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


struct TextureAsset : BaseAsset{

    uint32_t width{};
    uint32_t height{};
    uint32_t numChannel{};
    TextureImport textureImport{};

    unsigned char* data{};
};

namespace Asset_Impl {
    struct TextureAsset_Impl : BaseAsset {
        uint32_t width;
        uint32_t height;
        uint32_t numChannel;
        TextureImport textureImport;

        std::string binPath{}; //方便序列化
    };

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TextureAsset_Impl, name, contentHash, fileTime,
        sourcePath, width, height, numChannel, textureImport, binPath)
}


inline void SaveTextureBin(const std::string& path, unsigned char* data, uint32_t totalSize) {
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(data), totalSize);
}

inline unsigned char* LoadTextureBin(const std::string& path, uint32_t totalSize) {
    std::ifstream file(path, std::ios::binary);
    unsigned char* data = new unsigned char[totalSize];
    if (file.is_open()) {
        file.read(reinterpret_cast<char*>(data), totalSize);
    }else {
        throw std::runtime_error("Could not open file to load Texture Bin" + path);
        return nullptr;
    }
    return data;
}

#endif //CAIENGINE_TEXTRUEASSET_H
