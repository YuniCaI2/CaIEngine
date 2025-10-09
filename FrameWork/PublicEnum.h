//
// Created by 51092 on 25-5-30.
//

#ifndef PUBLICENUM_H
#define PUBLICENUM_H
#include<iostream>
#include <nlohmann/json.hpp>

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


#endif //PUBLICENUM_H


