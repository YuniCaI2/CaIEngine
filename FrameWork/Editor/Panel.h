//
// Created by cai on 2025/11/1.
//

#ifndef CAIENGINE_PANEL_H
#define CAIENGINE_PANEL_H
#include "imgui.h"

namespace Editor {
    class Panel {
    public:
        virtual void Tick(const float& tick) = 0;
        virtual ImGuiID GetID() = 0;
        virtual Render() = 0;
        virtual ~Panel() = default;
    private:
    };
}

#endif //CAIENGINE_PANEL_H