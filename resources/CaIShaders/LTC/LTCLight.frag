#version 450 core

layout (location = 2) in vec2 vTexCoord;
layout (location = 4) in float vIntensity;

layout(location = 0) out vec4 outColor;


layout (binding = 1) uniform sampler2D colorSampler;



        void main()
        {
            vec3 baseColor = vec3(texture(colorSampler, vTexCoord));
            outColor = vec4(vec3(baseColor * vIntensity), 1.0);
        }
    