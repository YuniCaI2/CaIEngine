#version 450

// 顶点着色器的输入属性
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inTangent;
layout (location = 3) in vec2 inTexCoord;

// 全局变换矩阵
layout (set = 0,binding = 0) uniform globalUBO
{
    mat4 viewMatrix;
    mat4 projectionMatrix;
} globalUbo;

layout (set = 5, binding = 0) uniform modelUBO
{
    mat4 modelMatrix;
} modelUbo;


// 基本顶点数据输出
layout (location = 0) out vec2 outTexCoord;
layout (location = 1) out vec3 outWorldPos;
layout (location = 2) out vec3 outNormal;

void main()
{
    vec4 worldPos = modelUbo.modelMatrix * vec4(inPos, 1.0);
    outWorldPos = vec3(worldPos);
    gl_Position = globalUbo.projectionMatrix * globalUbo.viewMatrix * worldPos;

    // 正确计算法线变换（应该使用法线矩阵）
    mat3 normalMatrix = transpose(inverse(mat3(modelUbo.modelMatrix)));
    outNormal = normalMatrix * inNormal;

    outTexCoord = inTexCoord;


}