#version 450 core

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec4 inColor;

layout(location = 0) out vec4 outColor;

layout (binding = 1) uniform UniformBufferObject {
    vec4 tintColor;
    float brightness;
} _UBO;

layout (binding = 2) uniform sampler2D mainTexture;

        void main(){
            vec4 texColor = texture(mainTexture, inUV);
            outColor = texColor * inColor * _UBO.tintColor;
            outColor.rgb *= _UBO.brightness;
        }
    