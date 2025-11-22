//
// Created by 51092 on 25-5-30.
//

#ifndef PUBLICENUM_H
#define PUBLICENUM_H
#include <nlohmann/json.hpp>
#include<expected>

template<class T>
using ExpectWithStr = std::expected<T, std::string>;



template<class T = int>
struct ErrorInfo { 
    std::string msg{};
    T code{};
    ErrorInfo(const std::string& msg, T code) : msg(msg), code(code) {}
    ErrorInfo(const std::string& msg) : msg(msg) {}
};
ErrorInfo(const std::string& msg) ->ErrorInfo<int>;

template<class T, class CodeType = int>
using ExpectedWithInfo = std::expected<T, ErrorInfo<CodeType>>;

template<class CodeType = int>
using UnexpectedWithInfo = std::unexpected<ErrorInfo<CodeType>>;


enum class MouseButton {
    Mid,
    Right,
    Left
};

enum Key {
    //数字键
    Key_0 = 0,
    Key_1,
    Key_2,
    Key_3,
    Key_4,
    Key_5,
    Key_6,
    Key_7,
    Key_8,
    Key_9,

    //字母键位
    Key_A,
    Key_B,
    Key_C,
    Key_D,
    Key_E,
    Key_F,
    Key_G,
    Key_H,
    Key_I,
    Key_J,
    Key_K,
    Key_L,
    Key_M,
    Key_N,
    Key_O,
    Key_P,
    Key_Q,
    Key_R,
    Key_S,
    Key_T,
    Key_U,
    Key_V,
    Key_W,
    Key_X,
    Key_Y,
    Key_Z,

    //方向键
    Key_Right,
    Key_Left,
    Key_Down,
    Key_Up,
    //空格
    Key_Space,
    Key_LeftShift,
    Key_RightShift,

    //退出
    Key_Escape,
};

enum ShaderType {
    Comp = 1 << 0,
    Vertex = 1 << 2,
    Frag = 1 << 3,
};
using ShaderTypeFlags = uint32_t;

enum class DescriptorType {
    UniformDynamic,
    Texture,
    Uniform,
    Storage
};

enum class RenderQueueType {
    Opaque = 0,
    Transparent = 1,
};

NLOHMANN_JSON_SERIALIZE_ENUM(RenderQueueType,
    {{RenderQueueType::Opaque, "Opaque"},
    {RenderQueueType::Transparent, "Transparent"}}
    )

enum class AttachmentType {
    Present,
    Color,
    Depth
};

enum class BlendOp {
    Opaque,
    Transparent,
    Multiply
};

enum TextureTypeFlagBits : uint32_t{
    DiffuseColor = 1 << 0,
    Normal = 1 << 1,
    MetallicRoughness = 1 << 2,
    Emissive = 1 << 3,
    Occlusion = 1 << 4,
    Ambient = 1 << 5,
    BaseColor = 1 << 6,
    SFLOAT16 = 1 << 7, //对应有负数的纹理
    SFLOAT32 = 1 << 8,
    None = 1 << 9,
};
NLOHMANN_JSON_SERIALIZE_ENUM(TextureTypeFlagBits,
    {
        {DiffuseColor, "DiffuseColor"},
        {Normal, "Normal"},
        {MetallicRoughness, "MetallicRoughness"},
        {Emissive, "Emissive"},
        {Occlusion, "Occlusion"},
        {Ambient, "Ambient"},
        {BaseColor, "BaseColor"},
        {SFLOAT16, "SFloat16"},
        {SFLOAT32, "SFloat32"}
    }
    )
//导入顺序也同上，当然不存在空缺
using TextureTypeFlags = uint32_t;

enum class ModelType {
    OBJ,
    GLTF,
    FBX,
    GLB,
};

enum class LightType {
    DOT,
    SPOT,
    FACE
};


enum class LogLevel {
    Trace,
    Debug,
    Warn,
    Error
};


enum class ShaderPropertyType
{
    BOOL,
    INT,
    UINT,
    FLOAT,
    VEC2,
    VEC3,
    VEC4,
    IVEC2,
    IVEC3,
    IVEC4,
    UVEC2,
    UVEC3,
    UVEC4,
    MAT2,
    MAT3,
    MAT4,

    SAMPLER,
    SAMPLER_2D,
    SAMPLER_CUBE,
};

enum class ShaderFormat {
    R8G8B8A8_UNORM,
    R8G8B8A8_SRGB,
    R16G16B16A16_SFLOAT,
    SWAPCHAIN_FORMAT
};

enum class CompareOption
{
    NEVER,
    LESS,
    EQUAL,
    LESS_OR_EQUAL,
    GREATER,
    NOT_EQUAL,
    GREATER_OR_EQUAL,
    ALWAYS,
};


enum class BlendOption
{
    ADD,
    SUBTRACT,
    REVERSE_SUBTRACT,
    MIN,
    MAX,
};


enum class BlendFactor
{
    ZERO,
    ONE,
    SRC_COLOR,
    ONE_MINUS_SRC_COLOR,
    DST_COLOR,
    ONE_MINUS_DST_COLOR,
    SRC_ALPHA,
    ONE_MINUS_SRC_ALPHA,
    DST_ALPHA,
    ONE_MINUS_DST_ALPHA,
    CONSTANT_COLOR,
    ONE_MINUS_CONSTANT_COLOR,
    CONSTANT_ALPHA,
    ONE_MINUS_CONSTANT_ALPHA
};

enum class FaceCullOption {
    None,
    Front,
    Back,
    FrontAndBack
};

enum class RenderPassType {
    Present,
    Forward,
    MsaaForward,
    Normal,
    Color,
    GBuffer,
    Deferred,
    MAX
};

enum class PolygonMode {
    Line,
    Fill
};

enum class SSBO_OP {
    Write,
    Read,
    WriteRead
};

enum class StorageObjectType {
    Image2D,
    Image3D,
    ImageCube,
    Buffer
};

enum class StorageImageFormat {
    RGBA8,
    RGBA16F
};

// ==================== ShaderPropertyType ====================
NLOHMANN_JSON_SERIALIZE_ENUM(ShaderPropertyType, {
    {ShaderPropertyType::BOOL, "BOOL"},
    {ShaderPropertyType::INT, "INT"},
    {ShaderPropertyType::UINT, "UINT"},
    {ShaderPropertyType::FLOAT, "FLOAT"},
    {ShaderPropertyType::VEC2, "VEC2"},
    {ShaderPropertyType::VEC3, "VEC3"},
    {ShaderPropertyType::VEC4, "VEC4"},
    {ShaderPropertyType::IVEC2, "IVEC2"},
    {ShaderPropertyType::IVEC3, "IVEC3"},
    {ShaderPropertyType::IVEC4, "IVEC4"},
    {ShaderPropertyType::UVEC2, "UVEC2"},
    {ShaderPropertyType::UVEC3, "UVEC3"},
    {ShaderPropertyType::UVEC4, "UVEC4"},
    {ShaderPropertyType::MAT2, "MAT2"},
    {ShaderPropertyType::MAT3, "MAT3"},
    {ShaderPropertyType::MAT4, "MAT4"},
    {ShaderPropertyType::SAMPLER, "SAMPLER"},
    {ShaderPropertyType::SAMPLER_2D, "SAMPLER_2D"},
    {ShaderPropertyType::SAMPLER_CUBE, "SAMPLER_CUBE"}
})

// ==================== ShaderFormat ====================
NLOHMANN_JSON_SERIALIZE_ENUM(ShaderFormat, {
    {ShaderFormat::R8G8B8A8_UNORM, "R8G8B8A8_UNORM"},
    {ShaderFormat::R8G8B8A8_SRGB, "R8G8B8A8_SRGB"},
    {ShaderFormat::R16G16B16A16_SFLOAT, "R16G16B16A16_SFLOAT"},
    {ShaderFormat::SWAPCHAIN_FORMAT, "SWAPCHAIN_FORMAT"}
})

// ==================== CompareOption ====================
NLOHMANN_JSON_SERIALIZE_ENUM(CompareOption, {
    {CompareOption::NEVER, "NEVER"},
    {CompareOption::LESS, "LESS"},
    {CompareOption::EQUAL, "EQUAL"},
    {CompareOption::LESS_OR_EQUAL, "LESS_OR_EQUAL"},
    {CompareOption::GREATER, "GREATER"},
    {CompareOption::NOT_EQUAL, "NOT_EQUAL"},
    {CompareOption::GREATER_OR_EQUAL, "GREATER_OR_EQUAL"},
    {CompareOption::ALWAYS, "ALWAYS"}
})

// ==================== BlendOption ====================
NLOHMANN_JSON_SERIALIZE_ENUM(BlendOption, {
    {BlendOption::ADD, "ADD"},
    {BlendOption::SUBTRACT, "SUBTRACT"},
    {BlendOption::REVERSE_SUBTRACT, "REVERSE_SUBTRACT"},
    {BlendOption::MIN, "MIN"},
    {BlendOption::MAX, "MAX"}
})

// ==================== BlendFactor ====================
NLOHMANN_JSON_SERIALIZE_ENUM(BlendFactor, {
    {BlendFactor::ZERO, "ZERO"},
    {BlendFactor::ONE, "ONE"},
    {BlendFactor::SRC_COLOR, "SRC_COLOR"},
    {BlendFactor::ONE_MINUS_SRC_COLOR, "ONE_MINUS_SRC_COLOR"},
    {BlendFactor::DST_COLOR, "DST_COLOR"},
    {BlendFactor::ONE_MINUS_DST_COLOR, "ONE_MINUS_DST_COLOR"},
    {BlendFactor::SRC_ALPHA, "SRC_ALPHA"},
    {BlendFactor::ONE_MINUS_SRC_ALPHA, "ONE_MINUS_SRC_ALPHA"},
    {BlendFactor::DST_ALPHA, "DST_ALPHA"},
    {BlendFactor::ONE_MINUS_DST_ALPHA, "ONE_MINUS_DST_ALPHA"},
    {BlendFactor::CONSTANT_COLOR, "CONSTANT_COLOR"},
    {BlendFactor::ONE_MINUS_CONSTANT_COLOR, "ONE_MINUS_CONSTANT_COLOR"},
    {BlendFactor::CONSTANT_ALPHA, "CONSTANT_ALPHA"},
    {BlendFactor::ONE_MINUS_CONSTANT_ALPHA, "ONE_MINUS_CONSTANT_ALPHA"}
})

// ==================== FaceCullOption ====================
NLOHMANN_JSON_SERIALIZE_ENUM(FaceCullOption, {
    {FaceCullOption::None, "None"},
    {FaceCullOption::Front, "Front"},
    {FaceCullOption::Back, "Back"},
    {FaceCullOption::FrontAndBack, "FrontAndBack"}
})

// ==================== RenderPassType ====================
NLOHMANN_JSON_SERIALIZE_ENUM(RenderPassType, {
    {RenderPassType::Present, "Present"},
    {RenderPassType::Forward, "Forward"},
    {RenderPassType::MsaaForward, "MsaaForward"},
    {RenderPassType::Normal, "Normal"},
    {RenderPassType::Color, "Color"},
    {RenderPassType::GBuffer, "GBuffer"},
    {RenderPassType::Deferred, "Deferred"},
    {RenderPassType::MAX, "MAX"}
})

// ==================== PolygonMode ====================
NLOHMANN_JSON_SERIALIZE_ENUM(PolygonMode, {
    {PolygonMode::Line, "Line"},
    {PolygonMode::Fill, "Fill"}
})

// ==================== SSBO_OP ====================
NLOHMANN_JSON_SERIALIZE_ENUM(SSBO_OP, {
    {SSBO_OP::Write, "Write"},
    {SSBO_OP::Read, "Read"},
    {SSBO_OP::WriteRead, "WriteRead"}
})

// ==================== StorageObjectType ====================
NLOHMANN_JSON_SERIALIZE_ENUM(StorageObjectType, {
    {StorageObjectType::Image2D, "Image2D"},
    {StorageObjectType::Image3D, "Image3D"},
    {StorageObjectType::ImageCube, "ImageCube"},
    {StorageObjectType::Buffer, "Buffer"}
})

// ==================== StorageImageFormat ====================
NLOHMANN_JSON_SERIALIZE_ENUM(StorageImageFormat, {
    {StorageImageFormat::RGBA8, "RGBA8"},
    {StorageImageFormat::RGBA16F, "RGBA16F"}
})


#endif //PUBLICENUM_H
