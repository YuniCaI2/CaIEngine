#version 450 core

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inTangent;
layout (location = 3) in vec2 inTexCoord;

layout(location = 0) out vec2 outTexCoord;
layout(location = 1) out vec3 outWorldPos;
layout(location = 2) out vec3 outNormal;

layout (binding = 0) uniform UniformBufferObject {
    mat4 viewMatrix;
    mat4 projectionMatrix;
    mat4 modelMatrix;
} _UBO;


        void main()
        {
            vec4 worldPos = _UBO.modelMatrix * vec4(inPos, 1.0);
            outWorldPos = vec3(worldPos);
            gl_Position = _UBO.projectionMatrix * _UBO.viewMatrix * worldPos;

            // 正确计算法线变换（应该使用法线矩阵）
            mat3 normalMatrix = transpose(inverse(mat3(_UBO.modelMatrix)));
            outNormal = normalMatrix * inNormal;

            outTexCoord = inTexCoord;
        }
    