#version 450

// 从顶点着色器接收的输入
layout(location = 0) in vec3 fragColor;

// 输出颜色
layout(location = 0) out vec4 outColor;




void main() {
    // 基础颜色
    vec3 color = fragColor;
    // 输出最终颜色
    outColor = vec4(color, 1.0);
}