#version 450


layout (location = 0) in vec3 inWorldPos;     // 接收世界空间位置 (本着色器未使用)
layout (location = 1) in vec3 inWorldNormal;  // 接收世界空间法线 (本着色器未使用)
layout (location = 2) in vec2 inTexCoord;     // 接收纹理坐标 (将要使用!)
layout (location = 3) in mat3 inTBN;          // 接收TBN矩阵 (本着色器未使用)


layout (set = 1, binding = 0) uniform sampler2D colorSampler;
layout (set = 2, binding = 0) uniform sampler2D normalSampler;

layout (location = 0) out vec4 outColor;

void main()
{
  outColor = texture(normalSampler, inTexCoord);
}