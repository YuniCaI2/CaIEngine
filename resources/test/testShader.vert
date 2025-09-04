#version 450 core

layout (location = 0) in vec2 inPos;
layout (location = 1) in vec2 inUv;
layout (location = 2) in vec4 inColor;

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec4 outColor;

layout (binding = 0) uniform UniformBufferObject {
    vec2 scale;
    vec2 translate;
} _UBO;


        void main(){
            outUV = inUv;
            outColor = inColor;
            gl_Position = vec4(inPos * _UBO.scale + _UBO.translate, 0.0, 1.0);
        }
    