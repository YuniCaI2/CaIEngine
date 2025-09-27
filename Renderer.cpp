//
// Created by AI Assistant on 25-5-26.
//
#include <vulkanFrameWork.h>

#include "BaseScene.h"
#include "FrameWorkGUI.h"
#include "Logger.h"
#include "Scene.h"
#include "Timer.h"
#include "VulkanDebug.h"
#include "VulkanWindow.h"
#include "Scene/LTCScene.h"

class Renderer {
private:
    FrameWork::Camera camera{};
    FrameWork::Timer frameTimer;
    FrameWork::Timer cameraTimer;
    FrameWork::FrameWorkGUI GUI{};
    std::vector<std::unique_ptr<FrameWork::Scene> > scenes{};
    uint32_t currentSceneIndex = 0;

public:
    friend struct UniformBufferObject;

    Renderer() {
        vulkanRenderAPI.SetTitle("Triangle Renderer");
        camera.Position = glm::vec3(0.0f, 0.0f, 1.0f);
    }

    ~Renderer() {
        GUI.ReleaseGUIResources();
    }

    void buildCommandBuffers() {
        auto cmdBuffer = vulkanRenderAPI.BeginCommandBuffer();
        scenes[currentSceneIndex]->Render(cmdBuffer);
        GUI.RenderGUI(cmdBuffer);
        vulkanRenderAPI.EndCommandBuffer();
    }



    void prepare() {
        //创建场景
        // auto scene1 = std::make_unique<BaseScene>(camera);
        // scenes.push_back(std::move(scene1));
#ifdef _WIN32
        auto scene2 = std::make_unique<LTCScene>(&camera);
        scenes.push_back(std::move(scene2));
#endif
        GUI.InitFrameWorkGUI();
        SetGUI();
    }

    void render() {
        // LOG_DEBUG("Elapsed Time : {}", timer.GetElapsedSeconds());
        camera.update(cameraTimer.GetElapsedSeconds());
        cameraTimer.Restart();
        // vulkanRenderAPI.UpdateAllSlots();
        vulkanRenderAPI.prepareFrame(frameTimer.GetElapsedMilliTime());
        frameTimer.Restart();
        buildCommandBuffers();
        vulkanRenderAPI.submitFrame();
    }

    void SetGUI() {
        GUI.SetGUIItems(
            [this] {
                if (ImGui::BeginCombo("Current Scene", scenes[currentSceneIndex]->GetName().c_str())) {
                    for (int i = 0; i < scenes.size(); i++) {
                        bool isSelected = (i == currentSceneIndex);
                        if (ImGui::Selectable(scenes[i]->GetName().c_str(), isSelected)) {
                            currentSceneIndex = i;
                            camera.reset({0,0,3});

                        }
                        if (isSelected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }

                ImGui::Separator();

                // 渲染当前场景的控制界面
                scenes[currentSceneIndex]->GetRenderFunction()();
            }
    );
}

};

int main() {
    vulkanRenderAPI.initVulkan();
    LOG.Run();
    LOG.SetPrintToFile(false);
    {
        Renderer app;
        app.prepare();
        auto &inputManager = FrameWork::InputManager::GetInstance();
        WINDOW_LOOP(
            inputManager.update();
            app.render();
        )
    }
    vulkanRenderAPI.DestroyAll();
    LOG.Stop();
    return 0;
}
