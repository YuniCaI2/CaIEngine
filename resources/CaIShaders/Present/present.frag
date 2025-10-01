#version 450 core

layout (location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 fragColor;


layout (binding = 0) uniform sampler2D colorTexture;

        vec3 ACEStonemap(vec3 hdr) {
            float a = 2.51;
            float b = 0.03;
            float c = 2.43;
            float d = 0.59;
            float e = 0.14;
            return clamp((hdr * (a * hdr + b)) / (hdr * (c * hdr + d) + e), 0.0, 1.0);
        }

        void main() {
            // 采样上一个pass的颜色
            fragColor = textureLod(colorTexture, fragTexCoord, 0);
            vec3 hdr = vec3(fragColor.r, fragColor.g, fragColor.b);
            hdr = ACEStonemap(hdr);
            //fragColor = vec4(hdr, 1.0);
        }
    