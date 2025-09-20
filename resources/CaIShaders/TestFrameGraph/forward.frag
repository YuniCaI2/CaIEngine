#version 450 core


layout(location = 0) out vec4 outColor;

layout (binding = 0) uniform UniformBufferObject {
    vec3 color0;
    vec4 color1;
    vec3 color2;
} _UBO;


        void main()
        {
            outColor = vec4(_UBO.color0 , 1.0);
        }
    