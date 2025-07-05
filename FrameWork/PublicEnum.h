//
// Created by 51092 on 25-5-30.
//

#ifndef PUBLICENUM_H
#define PUBLICENUM_H

enum class MouseButton {
    Mid,
    Right,
    Left
};

enum Key {
    //数字键
    Key_0 = 0,
    Key_1,
    Key_2,
    Key_3,
    Key_4,
    Key_5,
    Key_6,
    Key_7,
    Key_8,
    Key_9,

    //字母键位
    Key_A,
    Key_B,
    Key_C,
    Key_D,
    Key_E,
    Key_F,
    Key_G,
    Key_H,
    Key_I,
    Key_J,
    Key_K,
    Key_L,
    Key_M,
    Key_N,
    Key_O,
    Key_P,
    Key_Q,
    Key_R,
    Key_S,
    Key_T,
    Key_U,
    Key_V,
    Key_W,
    Key_X,
    Key_Y,
    Key_Z,

    //方向键
    Key_Right,
    Key_Left,
    Key_Down,
    Key_Up,
    //空格
    Key_Space,
    Key_LeftShift,
    Key_RightShift,

    //退出
    Key_Escape,
};

enum class ShaderType {
    Comp,
    Vertex,
    Frag,
    Count
};

enum class DescriptorType {
    Uniform,
    Texture
};


#endif //PUBLICENUM_H


