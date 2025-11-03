//
// Created by cai on 2025/11/1.
//

#ifndef CAIENGINE_PANEL_H
#define CAIENGINE_PANEL_H
#include "imgui.h"
#include<string>

namespace Editor {

    class Panel {
    public:
        virtual ~Panel() = default;
        const std::string& GetName() const { return name; }
        ImGuiID GetID() const { return ImGui::GetID(name.c_str()); }
        virtual void Render() = 0;
    protected:
        std::string name;
    };

} // namespace Editor

#endif //CAIENGINE_PANEL_H