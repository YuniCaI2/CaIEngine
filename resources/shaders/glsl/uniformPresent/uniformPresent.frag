#version 450

// 输入的纹理坐标
layout(location = 0) in vec2 fragTexCoord;

// 采样器：绑定上一个pass的color attachment
layout(set = 0,binding = 0) uniform sampler2D colorTexture;

// 输出颜色
layout(location = 0) out vec4 fragColor;

void main() {
    // 采样上一个pass的颜色
    fragColor = texture(colorTexture, fragTexCoord);
}