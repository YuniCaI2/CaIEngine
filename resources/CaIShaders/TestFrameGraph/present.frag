#version 450 core

layout (location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 fragColor;


layout (binding = 0) uniform sampler2D colorTexture;

        void main() {
            // 采样上一个pass的颜色
            fragColor = texture(colorTexture, fragTexCoord);
        }
    