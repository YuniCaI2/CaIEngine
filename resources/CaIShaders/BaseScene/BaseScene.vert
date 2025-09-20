#version 450 core

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inTangent;
layout (location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outWorldNormal;
layout(location = 2) out vec2 outTexCoord;
layout(location = 3) out mat3 outTBN;

layout (binding = 0) uniform UniformBufferObject {
    mat4 viewMatrix;
    mat4 projectionMatrix;
    mat4 modelMatrix;
} _UBO;


        void main()
        {
            vec4 worldPos = _UBO.modelMatrix * vec4(inPos, 1.0);
            outWorldPos = worldPos.xyz;

            gl_Position = _UBO.projectionMatrix * _UBO.viewMatrix * worldPos;

            outWorldNormal = normalize(transpose(inverse(mat3(_UBO.modelMatrix))) * inNormal);
            vec3 worldTangent = normalize(mat3(_UBO.modelMatrix) * inTangent);

            vec3 worldBitangent = cross(outWorldNormal, worldTangent);
            outTBN = mat3(worldTangent, worldBitangent, outWorldNormal);

            outTexCoord = inTexCoord;
        }
    