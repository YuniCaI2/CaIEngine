//
// Created by 51092 on 2025/10/9.
//


#include "Logger.h"
#include "ECS/Components/Renderable.h"
#include "ECS/Components/Translate.h"
#include "PublicStruct.h"
#include <Serialize.h>



struct TestComponent {
    int x;
    int y;
    int z;
};

void FixParent(FrameWork::PrefabStruct* node) {
    for (auto& child : node->children) {
        child->parent = node;
        FixParent(child.get());
    }
}

void testPrefabSerialize() {
    auto root = std::make_unique<FrameWork::PrefabStruct>();
    root->name = "Root";
    root->modelDataPath = "models/robot/robot_lod0.mesh";
    root->materialData = std::make_unique<FrameWork::MaterialStruct>(
        FrameWork::MaterialStruct{ "RootMat", "materials/root.json", "shaders/pbr.vert", "void main(){}" }
    );

    auto c1 = std::make_unique<FrameWork::PrefabStruct>();
    c1->name = "Arm";
    c1->modelDataPath = "models/robot/arm.mesh";

    auto c2 = std::make_unique<FrameWork::PrefabStruct>();
    c2->name = "Head";
    c2->modelDataPath = "models/robot/head.mesh";

    root->children.push_back(std::move(c1));
    root->children.push_back(std::move(c2));
    FixParent(root.get()); // 运行期关系

    // --- 序列化到 JSON ---
    nlohmann::json j = *root;
    std::cout << "Serialized:\n" << j.dump(2) << "\n\n";

    // --- 反序列化回对象（仍然只有路径，没有 ModelData 实体）---
    FrameWork::PrefabStruct loaded = j.get<FrameWork::PrefabStruct>();
    FixParent(&loaded);

}

int main() {
    ECS::Translate translate;
    translate.rotation[0][0] = 1;
    translate.rotation[0][1] = 1;
    translate.rotation[0][2] = 1;
    translate.rotation[0][3] = 1;
    ECS::Renderable renderable;
    nlohmann::json render = renderable;
    nlohmann::json trans = translate;
    nlohmann::json vec3 = glm::vec3(1,1,1);
    std::cout << render << std::endl;
    testPrefabSerialize();
}