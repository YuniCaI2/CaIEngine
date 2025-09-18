#include <FrameGraph/FrameGraph.h>
#include <FrameGraph/ResourceManager.h>
#include <FrameGraph/RenderPassManager.h>
#include <vulkanFrameWork.h>
#include <Logger.h>

#include "Timer.h"
#include "VulkanWindow.h"

using namespace FG;

void TestFrameGraph() {
    ResourceManager resourceManager;
    RenderPassManager renderPassManager;
    FrameGraph frameGraph(resourceManager, renderPassManager);
    std::string ForwardPath = "../../resources/CaIShaders/TestFrameGraph/forward.caishader";
    std::string PresentPath = "../../resources/CaIShaders/TestFrameGraph/present.caishader";
    uint32_t forwardShader = -1;
    uint32_t presentShader = -1;
    FrameWork::CaIShader::Create(forwardShader, ForwardPath);
    FrameWork::CaIShader::Create(presentShader, PresentPath);


    // 创建Vulkan资源
    auto &api = vulkanRenderAPI;
    uint32_t presentMaterial = -1;
    FrameWork::CaIMaterial::Create(presentMaterial, presentShader);
    auto swapChainTex = api.CreateSwapChainTexture();

    //注册资源
    auto colorAttachment = resourceManager.RegisterResource([&](std::unique_ptr<ResourceDescription> &desc) {
            desc->SetName("colorAttachment")
                    .SetDescription<TextureDescription>(
                        std::make_unique<TextureDescription>(
                            api.windowWidth, api.windowHeight, api.GetVulkanSwapChain().colorFormat, 1, 1,
                            VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
                        )
                    );
        }
    );

    auto swapChainAttachment = resourceManager.RegisterResource([&](std::unique_ptr<ResourceDescription> &desc) {
        desc->SetName("swapChain");
        desc->SetDescription<TextureDescription>(std::make_unique<TextureDescription>());
        desc->isExternal = true;
        desc->isPresent = true;
        desc->vulkanIndex = swapChainTex[0]; //先随便绑定一个
    });

    //注册RenderPass
    auto forwardPass = renderPassManager.RegisterRenderPass([&](std::unique_ptr<RenderPass> &renderPass) {
        renderPass->SetName("forwardPass");
        renderPass->SetExec([&](VkCommandBuffer cmdBuffer) {
            FrameWork::CaIShader::Get(forwardShader)->Bind(cmdBuffer);
            vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
        });
    });

    auto presentPass = renderPassManager.RegisterRenderPass([&](auto &renderPass) {
        renderPass->SetName("presentPass");
        renderPass->SetExec([&](VkCommandBuffer cmdBuffer) {
            //绑定对应imageView
            FrameWork::CaIShader::Get(presentShader)->Bind(cmdBuffer);
            FrameWork::CaIMaterial::Get(presentMaterial)->SetTexture("colorTexture", resourceManager.GetVulkanResource(colorAttachment));
            FrameWork::CaIMaterial::Get(presentMaterial)->Bind(cmdBuffer);
            vkCmdDraw(cmdBuffer, 6, 1, 0, 0);
        });
    });
    //构建图
    frameGraph.AddResourceNode(colorAttachment).AddResourceNode(swapChainAttachment)
            .AddRenderPassNode(forwardPass).AddRenderPassNode(presentPass);
    frameGraph.SetUpdateBeforeRendering([&]() {
        resourceManager.FindResource(swapChainAttachment)->vulkanIndex = swapChainTex[vulkanRenderAPI.GetCurrentImageIndex()];
    });

    renderPassManager.FindRenderPass(forwardPass)->SetCreateResource(colorAttachment);
    renderPassManager.FindRenderPass(presentPass)->SetCreateResource(swapChainAttachment).SetReadResource(
        colorAttachment);

    resourceManager.FindResource(colorAttachment)->SetOutputRenderPass(forwardPass).SetInputRenderPass(presentPass);
    resourceManager.FindResource(swapChainAttachment)->SetOutputRenderPass(presentPass);
    frameGraph.Compile();


    //创建简单的前向渲染
    FrameWork::Timer timer;
    auto &inputManager = FrameWork::InputManager::GetInstance();
    WINDOW_LOOP(
        inputManager.update();
        vulkanRenderAPI.prepareFrame(timer.GetElapsedMilliTime());
        auto commandBuffer = api.BeginCommandBuffer();
        frameGraph.Execute(commandBuffer);
        api.EndCommandBuffer();
        vulkanRenderAPI.submitFrame();
        timer.Restart();
        );
}

int main() {
    LOG.Run();
    LOG.SetPrintToFile(false);
    if (!vulkanRenderAPI.initVulkan()) {
        LOG_ERROR("Failed to init Vulkan, abort FrameGraph test");
        LOG.Stop();
        return -1;
    }
    TestFrameGraph();
    vulkanRenderAPI.DestroyAll();
    LOG.Stop();
    return 0;
}
