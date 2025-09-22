#version 450 core

layout (location = 0) in vec2 outTexCoord;
layout (location = 1) in vec3 outWorldPos;
layout (location = 2) in vec3 outNormal;

layout(location = 0) out vec4 fragColor;

layout (binding = 1) uniform UniformBufferObject {
    mat4 lightMatrix;
    vec3 lightPosition;
    float lightHeight;
    vec3 lightColor;
    float lightWidth;
    float lightIntensity;
    vec3 cameraPos;
    vec3 materialDiffuse;
    vec3 materialF0;
    float materialRoughness;
} _UBO;

layout (binding = 2) uniform sampler2D LTC1;
layout (binding = 3) uniform sampler2D LTC2;
layout (binding = 4) uniform sampler2D lightTexture;

        // LUT相关常量
        const float LUT_SIZE = 64.0;
        const float LUT_SCALE = (LUT_SIZE - 1.0)/LUT_SIZE;
        const float LUT_BIAS = 0.5/LUT_SIZE; //移到中心
        const float pi = 3.14159265;

        vec3 ACEStonemap(vec3 hdr) {
            float a = 2.51;
            float b = 0.03;
            float c = 2.43;
            float d = 0.59;
            float e = 0.14;
            return clamp((hdr * (a * hdr + b)) / (hdr * (c * hdr + d) + e), 0.0, 1.0);
        }

        vec3 IntegrateEdgeVec(vec3 v1, vec3 v2)
        {
            float x = dot(v1, v2);
            float y = abs(x);

            float a = 0.8543985 + (0.4965155 + 0.0145206*y)*y;
            float b = 3.4175940 + (4.1616724 + y)*y;
            float v = a / b;

            float theta_sintheta = (x > 0.0) ? v : 0.5*inversesqrt(max(1.0 - x*x, 1e-7)) - v;

            return cross(v1, v2)*theta_sintheta;
        }

        vec3 mul(mat3 a, vec3 b){
            return a * b;
        }

        vec3 getF(vec3 P, mat3 Minv, vec3 points[4]){
            vec3 L[4];
            L[0] = mul(Minv, points[0] - P);
            L[1] = mul(Minv, points[1] - P);
            L[2] = mul(Minv, points[2] - P);
            L[3] = mul(Minv, points[3] - P);

            L[0] = normalize(L[0]);
            L[1] = normalize(L[1]);
            L[2] = normalize(L[2]);
            L[3] = normalize(L[3]);
            vec3 vsum = vec3(0.0);

            vsum += IntegrateEdgeVec(L[0], L[1]);
            vsum += IntegrateEdgeVec(L[1], L[2]);
            vsum += IntegrateEdgeVec(L[2], L[3]);
            vsum += IntegrateEdgeVec(L[3], L[0]);

            return vsum;
        }

        vec3 LTC_Evaluate(vec3 N, vec3 V, vec3 P, mat3 Minv, vec3 points[4])
        {
            vec3 T1, T2;
            T1 = normalize(V - N*dot(V, N));
            T2 = cross(N, T1);

            Minv = Minv * transpose(mat3(T1, T2, N));

            vec3 vsum = getF(P, Minv, points);

            vec3 dir = points[0].xyz - P;
            vec3 lightNormal = cross(points[1] - points[0], points[3] - points[0]);
            bool behind = (dot(dir, lightNormal) < 0.0);

            float len = length(vsum);
            float z = (vsum.z)/len;

            if (behind)
                z = -z;

            vec2 uv = vec2(z*0.5 + 0.5, len);
            uv = uv*LUT_SCALE + LUT_BIAS;

            float scale = texture(LTC2, uv).w;

            float sum = scale * len;

            if (behind)
                sum = 0.0;

            vec3 Lo_i = vec3(sum, sum, sum);
            return Lo_i;
        }

        vec3 GetLightTex(vec3 points[4], mat3 Minv, vec3 N, vec3 V){
            //Tex
            vec3 T1, T2;
            T1 = normalize(V - N*dot(V, N));
            T2 = cross(N, T1);
            Minv = Minv * transpose(mat3(T1, T2, N));
            vec3 normF = getF(outWorldPos, Minv, points);
            Minv = inverse(Minv);
            normF = normalize(normF);
            normF = Minv * normF;

            vec3 lightNormal = cross(points[1] - points[0], points[3] - points[0]);
            float FdotLn = dot(normF, lightNormal);
            if(abs(FdotLn) < 1e-4){
                return vec3(0.01, 0.01, 0.01);
            }
            float t = (dot(lightNormal, points[0]) - dot(lightNormal, outWorldPos)) / FdotLn;
            if(t < 0.0){
                return vec3(0.01, 0.01, 0.01);
            }
            vec3 insertPoint = outWorldPos + t * normF;
            vec3 xAxis = normalize(points[2] - points[1]);
            vec3 yAxis = normalize(points[0] - points[1]);
            float texX = dot(insertPoint - points[1], xAxis);
            float texY = dot(insertPoint - points[1], yAxis);
            texX = clamp(texX / length(points[2] - points[1]), 0.0, 1.0);
            texY = clamp(texY / length(points[0] - points[1]), 0.0, 1.0);
            vec2 texUV = vec2(texX, texY);
            float lod = (sqrt(_UBO.materialRoughness) + sqrt(t) * sqrt(max(abs(texX - 0.5), abs(texY - 0.5)))) * 6.0;
            return vec3(texUV, t);
        }

        void main() {
            vec3 V = normalize(_UBO.cameraPos - outWorldPos);
            vec3 N = normalize(outNormal);

            float nDotV = max(dot(N, V), 0.001);

            vec2 uv = vec2(_UBO.materialRoughness, sqrt(1.0 - nDotV)); //使得cos接近1时密度大
            uv = clamp(uv, 0.0, 1.0) * LUT_SCALE + LUT_BIAS;

            vec4 t1 = texture(LTC1, uv);
            vec4 t2 = texture(LTC2, uv);

            mat3 Minv = mat3(
                vec3(t1.x, 0, t1.y),
                vec3(0, 1, 0),
                vec3(t1.z, 0, t1.w)
            );

            vec3 points[4];
            float halfH = _UBO.lightHeight * 0.5;
            float halfW = _UBO.lightWidth * 0.5;

            points[0] = vec3(-halfW, -halfH, 0.0);
            points[1] = vec3(-halfW,  halfH, 0.0);
            points[2] = vec3( halfW,  halfW, 0.0);
            points[3] = vec3( halfW, -halfH, 0.0);

            // 变换到世界空间
            for(int i = 0; i < 4; i++){
                vec4 worldPos = _UBO.lightMatrix * vec4(points[i], 1.0);
                points[i] = worldPos.xyz;
            }

            // 计算镜面反射
            vec3 spec = LTC_Evaluate(N, V, outWorldPos, Minv, points);
            spec *= _UBO.materialF0 * t2.x + (1.0 - _UBO.materialF0) * t2.y;
            vec3 textureRt = GetLightTex(points, Minv, N, V);
            vec2 texUV = vec2(textureRt.x ,textureRt.y);
            float t = textureRt.z;
            float lod = (sqrt(1.0 - max(1.0, length(spec))) + sqrt(_UBO.materialRoughness) * 6.0) * 2.0;
            spec *= vec3(textureLod(lightTexture, texUV, lod));

            vec3 diff = LTC_Evaluate(N, V, outWorldPos, mat3(1.0), points);

            vec3 radiance = _UBO.lightColor * _UBO.lightIntensity;
            vec3 col = radiance * (spec + _UBO.materialDiffuse * diff);

            fragColor = vec4(ACEStonemap(col), 1.0);
        }
    