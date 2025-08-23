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
layout (set = 1, binding = 0) uniform modelUBO
{
    mat4 modelMatrix;
} modelUbo;


layout (location = 0) out vec3 outWorldPos;   // 片段的世界空间位置
layout (location = 1) out vec3 outWorldNormal; // 片段的世界空间法线
layout (location = 2) out vec2 outTexCoord;   // 片段的纹理坐标
layout (location = 3) out mat3 outTBN;        // 用于法线贴图的TBN矩阵


void main()
{

    vec4 worldPos = modelUbo.modelMatrix * vec4(inPos, 1.0);
    outWorldPos = worldPos.xyz;

    gl_Position = globalUbo.projectionMatrix * globalUbo.viewMatrix * worldPos;

    outWorldNormal = normalize(transpose(inverse(mat3(modelUbo.modelMatrix))) * inNormal);
    vec3 worldTangent = normalize(mat3(modelUbo.modelMatrix) * inTangent);

    vec3 worldBitangent = cross(outWorldNormal, worldTangent);
    outTBN = mat3(worldTangent, worldBitangent, outWorldNormal);

    outTexCoord = inTexCoord;
}