#version 450 core

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec3 inWorldNormal;
layout (location = 2) in vec2 inTexCoord;
layout (location = 3) in mat3 inTBN;

layout(location = 0) out vec4 outColor;


layout (binding = 1) uniform sampler2D colorSampler;

        void main()
        {
            outColor = texture(colorSampler, inTexCoord) * 0.9;
        }
    