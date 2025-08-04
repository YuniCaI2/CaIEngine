#version 450

// 输入顶点属性
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

// 输出到片段着色器
layout(location = 0) out vec3 fragColor;

// 统一缓冲区对象
layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;

} ubo;

void main() {
    // 计算世界空间位置
    vec4 worldPosition = ubo.model * vec4(inPosition, 1.0);

    // 计算最终的屏幕空间位置
    gl_Position = ubo.proj * ubo.view * worldPosition;

    // 传递颜色到片段着色器
    fragColor = inColor;
}