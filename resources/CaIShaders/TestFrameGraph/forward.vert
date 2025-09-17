#version 450 core





        vec2 positions[3] = vec2[](
            vec2(-0.5, -0.5),  // 左下
            vec2( 0.5, -0.5),  // 右下
            vec2( 0.0,  0.5)   // 顶部
        );
        void main()
        {
            vec2 positions[3] = vec2[](
                vec2(-0.5, -0.5),  // 左下
                vec2( 0.5, -0.5),  // 右下
                vec2( 0.0,  0.5)   // 顶部
            );

            int vertexIndex = gl_VertexIndex;
            vec2 pos = positions[vertexIndex];

            gl_Position = vec4(pos, 0.0, 1.0);
        }
    