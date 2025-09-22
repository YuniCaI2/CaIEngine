#version 450 core

layout (location = 2) in vec2 vTexCoord;
layout (location = 4) in float vIntensity;

layout(location = 0) out vec4 outColor;


layout (binding = 1) uniform sampler2D colorSampler;

        vec3 ACEStonemap(vec3 hdr) {
            float a = 2.51;
            float b = 0.03;
            float c = 2.43;
            float d = 0.59;
            float e = 0.14;
            return clamp((hdr * (a * hdr + b)) / (hdr * (c * hdr + d) + e), 0.0, 1.0);
        }

        void main()
        {
            vec3 baseColor = vec3(texture(colorSampler, vTexCoord));
            outColor = vec4(ACEStonemap(baseColor * vIntensity), 1.0);
        }
    