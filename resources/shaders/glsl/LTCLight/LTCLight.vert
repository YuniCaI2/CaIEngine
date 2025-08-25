#version 450

// 依然是顶点着色器的输入属性
layout (location = 0) in vec3 inPos;      // 顶点位置 (模型空间)
layout (location = 1) in vec3 inNormal;   // 顶点法线 (模型空间)
layout (location = 2) in vec3 inTangent;  // 顶点切线 (模型空间)
layout (location = 3) in vec2 inTexCoord; // 顶点纹理坐标

// 全局变换矩阵
layout (binding = 0) uniform globalUBO
{
    mat4 viewMatrix;
    mat4 projectionMatrix;
} globalUbo;
layout (set = 1, binding = 0) uniform uniformLightUBO{
    mat4 lightMatrix;
    vec3 position;
    vec3 normal;
    float height;
    float width;
    float intensity;
    vec3 color;
} lightUbo;
layout (set = 2, binding = 0) uniform modelUBO
{
    mat4 modelMatrix;
} modelUbo;


layout (location = 2) out vec2 outTexCoord;
layout (location = 4) flat out float intensity;


void main()
{

    vec4 worldPos = lightUbo.lightMatrix * vec4(inPos, 1.0);
    gl_Position = globalUbo.projectionMatrix * globalUbo.viewMatrix * worldPos;
    intensity = lightUbo.intensity;
    outTexCoord = inTexCoord;
}