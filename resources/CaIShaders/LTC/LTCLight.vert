#version 450 core

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inTangent;
layout (location = 3) in vec2 inTexCoord;

layout(location = 2) out vec2 vTexCoord;
layout(location = 4) out float vIntensity;

layout (binding = 0) uniform UniformBufferObject {
    mat4 viewMatrix;
    mat4 projectionMatrix;
    mat4 modelMatrix;
    float intensity;
} _UBO;


        void main()
        {
            vec4 worldPos = _UBO.modelMatrix * vec4(inPos, 1.0);
            gl_Position = _UBO.projectionMatrix * _UBO.viewMatrix * worldPos;
            vTexCoord = inTexCoord;
            vIntensity = _UBO.intensity;
        }
    