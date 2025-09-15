//
// Created by cai on 2025/9/13.
//
#include<FrameGraph/ResourceManager.h>

#include "FrameGraph/FrameGraph.h"
#include "FrameGraph/RenderPassManager.h"

using namespace FG;

void TestResourceManager() {
    ResourceManager resourceManager;
    auto test = resourceManager.RegisterResource(
        [](std::unique_ptr<ResourceDescription>& desc) {
            desc->SetName("Test Desc")
            .SetDescription<TextureDescription>(
                std::make_unique<TextureDescription>(
                    100, 100, VK_FORMAT_R8G8B8A8_UNORM, 1, 1,
                    VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
                    )
                );
        }
        );
    auto desc = resourceManager.FindResource("Test Desc");
    auto texDecs = desc->GetDescription<TextureDescription>();
}

void TestCullPass() {
    ResourceManager resourceManager;
    RenderPassManager renderPassManager;
    FrameGraph frameGraph(resourceManager, renderPassManager);

    // ===== 构建资源节点 =====

    // 输入：顶点缓冲 (Proxy - 外部输入)
    auto vertexBufferID = resourceManager.RegisterResource(
        [](std::unique_ptr<ResourceDescription>& desc) {
            desc->SetName("VertexBuffer");
            auto proxy = std::make_unique<ProxyDescription>();
            proxy->proxyType = ProxyType::ImportBuffer;
            proxy->vulkanIndex = 1;
            desc->SetDescription<ProxyDescription>(std::move(proxy));
        });
    frameGraph.AddResourceNode(vertexBufferID);

    // 中间产物：GBuffer
    auto GBuffer1 = resourceManager.RegisterResource(
        [](std::unique_ptr<ResourceDescription>& desc) {
            desc->SetName("GBuffer1")
            .SetDescription<TextureDescription>(
                std::make_unique<TextureDescription>(
                    1920, 1080, VK_FORMAT_R8G8B8A8_UNORM, 1, 1,
                    VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT));
        });
    frameGraph.AddResourceNode(GBuffer1);

    auto GBuffer2 = resourceManager.RegisterResource(
        [](std::unique_ptr<ResourceDescription>& desc) {
            desc->SetName("GBuffer2")
            .SetDescription<TextureDescription>(
                std::make_unique<TextureDescription>(
                    1920, 1080, VK_FORMAT_R8G8B8A8_UNORM, 1, 1,
                    VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT));
        });
    frameGraph.AddResourceNode(GBuffer2);

    // 中间产物：光照结果
    auto lightingOutput = resourceManager.RegisterResource(
        [](std::unique_ptr<ResourceDescription>& desc) {
            desc->SetName("LightingOutput")
            .SetDescription<TextureDescription>(
                std::make_unique<TextureDescription>(
                    1920, 1080, VK_FORMAT_R8G8B8A8_UNORM, 1, 1,
                    VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT));
        });
    frameGraph.AddResourceNode(lightingOutput);

    // 调试输出 (Transient - 仅用于调试，可能会被剔除)
    auto debugOutput = resourceManager.RegisterResource(
        [](std::unique_ptr<ResourceDescription>& desc) {
            desc->SetName("DebugOutput")
            .SetDescription<TextureDescription>(
                std::make_unique<TextureDescription>(
                    512, 512, VK_FORMAT_R8G8B8A8_UNORM, 1, 1,
                    VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT));
        });
    frameGraph.AddResourceNode(debugOutput);

    // 最终输出：交换链 (Proxy - 最终显示目标)
    auto swapChainImage = resourceManager.RegisterResource(
        [](std::unique_ptr<ResourceDescription>& desc) {
            desc->SetName("SwapChainImage");
            auto proxy = std::make_unique<ProxyDescription>();
            proxy->proxyType = ProxyType::SwapChain;
            proxy->vulkanIndex = 0;
            desc->SetDescription<ProxyDescription>(std::move(proxy));
        });
    frameGraph.AddResourceNode(swapChainImage);

    // ===== 构建 Pass 节点 =====

    auto GBufferPass = renderPassManager.RegisterRenderPass(
        [](std::unique_ptr<RenderPass>& desc) {
            desc->SetName("GBufferPass");
        });
    frameGraph.AddRenderPassNode(GBufferPass);

    auto LightingPass = renderPassManager.RegisterRenderPass(
        [](std::unique_ptr<RenderPass>& desc) {
            desc->SetName("LightingPass");
        });
    frameGraph.AddRenderPassNode(LightingPass);

    auto PresentPass = renderPassManager.RegisterRenderPass(
        [](std::unique_ptr<RenderPass>& desc) {
            desc->SetName("PresentPass");
        });
    frameGraph.AddRenderPassNode(PresentPass);

    // ===== 建立连接关系 =====

    // GBufferPass: 读取顶点缓冲，输出到 GBuffer1 和 GBuffer2
    renderPassManager.FindRenderPass(GBufferPass)
        ->SetReadResource(vertexBufferID)
        .SetCreateResource(GBuffer1)
        .SetCreateResource(GBuffer2);

    // 设置资源的生产者
    resourceManager.FindResource(vertexBufferID)->SetInputRenderPass(GBufferPass);
    resourceManager.FindResource(GBuffer1)->SetOutputRenderPass(GBufferPass);
    resourceManager.FindResource(GBuffer2)->SetOutputRenderPass(GBufferPass);

    // LightingPass: 读取 GBuffer 和顶点缓冲，输出光照结果和调试信息
    renderPassManager.FindRenderPass(LightingPass)
        ->SetReadResource(vertexBufferID)
        .SetReadResource(GBuffer1)
        .SetReadResource(GBuffer2)
        .SetCreateResource(lightingOutput)
        .SetCreateResource(debugOutput);

    // 设置资源的生产者和消费者
    resourceManager.FindResource(lightingOutput)->SetOutputRenderPass(LightingPass);
    resourceManager.FindResource(debugOutput)->SetOutputRenderPass(LightingPass);
    resourceManager.FindResource(GBuffer1)->SetInputRenderPass(LightingPass);
    resourceManager.FindResource(GBuffer2)->SetInputRenderPass(LightingPass);

    // PresentPass: 读取光照结果，输出到交换链
    renderPassManager.FindRenderPass(PresentPass)
        ->SetReadResource(lightingOutput)
        .SetCreateResource(swapChainImage);  // 创建 Proxy 资源

    // 设置最终的连接
    resourceManager.FindResource(lightingOutput)->SetInputRenderPass(PresentPass);
    resourceManager.FindResource(swapChainImage)->SetOutputRenderPass(PresentPass);

    // ===== 执行剔除和编译 =====
    std::cout << "=== 执行 FrameGraph 剔除 ===" << std::endl;
    frameGraph.CullPassAndResource();

    // 预期结果：
    // 1. 所有 Pass 都应该保留（GBufferPass, LightingPass, PresentPass）
    // 2. debugOutput 可能被剔除（如果没有被标记为重要输出）
    // 3. swapChainImage 作为 Proxy 应该是剔除的起点

    std::cout << "=== 剔除完成，检查结果 ===" << std::endl;
    //测试TimeLine
    frameGraph.CreateTimeline();
    frameGraph.CreateVulkanResources();
    LOG_TRACE("FrameGraph Created");
}

int main() {
    LOG.Run();
    vulkanRenderAPI.initVulkan();
    // TestResourceManager();
    TestCullPass();
    vulkanRenderAPI.DestroyAll();
    LOG.Stop();
}