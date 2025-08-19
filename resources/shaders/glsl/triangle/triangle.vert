#version 450

// 依然是顶点着色器的输入属性
layout (location = 0) in vec3 inPos;      // 顶点位置 (模型空间)
layout (location = 1) in vec3 inNormal;   // 顶点法线 (模型空间)
layout (location = 2) in vec3 inTangent;  // 顶点切线 (模型空间)
layout (location = 3) in vec2 inTexCoord; // 顶点纹理坐标

// 全局变换矩阵
layout (binding = 0) uniform globalUBO
{

	mat4 viewMatrix;
	mat4 projectionMatrix;
} globalUbo;
layout (set = 1, binding = 0) uniform modelUBO
{
	mat4 modelMatrix;
} modelUbo;


layout (location = 0) out vec3 outWorldPos;   // 片段的世界空间位置
layout (location = 1) out vec3 outWorldNormal; // 片段的世界空间法线
layout (location = 2) out vec2 outTexCoord;   // 片段的纹理坐标
layout (location = 3) out mat3 outTBN;        // 用于法线贴图的TBN矩阵


void main()
{
	// 1. 计算顶点在世界空间中的位置
	//    vec4(inPos, 1.0) 将三维位置转为四维其次坐标
	vec4 worldPos = modelUbo.modelMatrix * vec4(inPos, 1.0);
	outWorldPos = worldPos.xyz;

	// 2. 计算最终在屏幕上的位置 (MVP变换)
	gl_Position = globalUbo.projectionMatrix * globalUbo.viewMatrix * worldPos;

	// 3. 将法线、切线从模型空间变换到世界空间
	//    我们使用 mat3(ubo.modelMatrix) 是因为它只包含旋转和缩放，移除了位移
	//    注意：如果 modelMatrix 包含非均匀缩放，正确做法是用 transpose(inverse(mat3(ubo.modelMatrix)))
	outWorldNormal = normalize(mat3(modelUbo.modelMatrix) * inNormal);
	vec3 worldTangent = normalize(mat3(modelUbo.modelMatrix) * inTangent);

	// 4. 计算副切线 (Bitangent) 并构建 TBN 矩阵
	//    TBN 矩阵用于将法线贴图中的法线从切线空间转换到世界空间
	vec3 worldBitangent = cross(outWorldNormal, worldTangent);
	outTBN = mat3(worldTangent, worldBitangent, outWorldNormal);

	// 5. 直接传递纹理坐标到片段着色器
	outTexCoord = inTexCoord;
}