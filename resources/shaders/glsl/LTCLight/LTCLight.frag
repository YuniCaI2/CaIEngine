#version 450
layout (location = 2) in vec2 inTexCoord;
layout (location = 4) flat in float intensity;


layout (set = 3, binding = 0) uniform sampler2D colorSampler;
layout (location = 0) out vec4 outColor;

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
    outColor = vec4(ACEStonemap(vec3(texture(colorSampler, inTexCoord))  * intensity), 1.0);
}