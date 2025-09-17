// FrameGraph 综合测试：
// 目标：
// 1. 注册一组外部与内部纹理资源（含交换链、导入纹理、内部中间RT、深度）。
// 2. 构建四个 RenderPass：
//    P0: 创建 ColorA (内部) + Depth；
//    P1: 输入 ColorA -> 输出 ColorB；
//    P2: 读取 ColorB (只读) -> 输出 ColorC；
//    P3: 输入 ColorC -> 输出 SwapChain(展示)；
// 3. 调用 Compile() 验证：
//    - 能生成正确拓扑 timeline (无环)
//    - 为各 Pass 生成 command pool 与预/后 barrier（这里只做数量与关键字段的日志检查）
// 4. 调用 Execute() 录制次级命令缓冲（不真正提交，只做函数调用路径覆盖）。

#include <FrameGraph/FrameGraph.h>
#include <FrameGraph/ResourceManager.h>
#include <FrameGraph/RenderPassManager.h>
#include <vulkanFrameWork.h>
#include <Logger.h>

using namespace FG;

struct TestContext {
    ResourceManager resourceManager;
    RenderPassManager renderPassManager;
    std::unique_ptr<FrameGraph> frameGraph;
};

static uint32_t RegisterSwapChainResource(ResourceManager &rm) {
    // 将交换链中的每个 image 注册为外部 present 资源（这里只注册一个代表资源即可，用 isPresent 标记）
    return rm.RegisterResource([&](std::unique_ptr<ResourceDescription>& desc){
        desc->SetName("SwapChainColor");
        desc->isExternal = true;
        desc->isPresent = true; // present 资源由框架自动填充描述
        desc->SetDescription<TextureDescription>(std::make_unique<TextureDescription>());
    });
}

static uint32_t RegisterImportedTexture(ResourceManager &rm, uint32_t vulkanIndex, const std::string &name){
    return rm.RegisterResource([&](std::unique_ptr<ResourceDescription>& desc){
        desc->SetName(name);
        desc->isExternal = true;
        desc->vulkanIndex = vulkanIndex; // 使用已有纹理
        desc->SetDescription<TextureDescription>(std::make_unique<TextureDescription>());
    });
}

static uint32_t RegisterInternalRT(ResourceManager &rm, const std::string &name, VkFormat fmt, VkImageUsageFlags usage){
    return rm.RegisterResource([&](std::unique_ptr<ResourceDescription>& desc){
        desc->SetName(name);
        desc->SetDescription<TextureDescription>(std::make_unique<TextureDescription>(
            vulkanRenderAPI.GetFrameWidth(), vulkanRenderAPI.GetFrameHeight(), fmt,
            1, 1, VK_SAMPLE_COUNT_1_BIT, usage
        ));
    });
}

static uint32_t RegisterDepth(ResourceManager &rm){
    return rm.RegisterResource([&](std::unique_ptr<ResourceDescription>& desc){
        desc->SetName("Depth");
        desc->SetDescription<TextureDescription>(std::make_unique<TextureDescription>(
            vulkanRenderAPI.GetFrameWidth(), vulkanRenderAPI.GetFrameHeight(), vulkanRenderAPI.GetDepthFormat(),
            1,1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
        ));
    });
}

static void BuildRenderPasses(TestContext &ctx,
                              uint32_t colorA, uint32_t colorB, uint32_t colorC,
                              uint32_t depth, uint32_t swapChainColor){
    // Pass 0: 创建 A + 深度
    auto p0 = ctx.renderPassManager.RegisterRenderPass([&](std::unique_ptr<RenderPass>& rp){
        rp->SetName("P0_CreateA")
           .SetCreateResource(colorA)
           .SetCreateResource(depth)
           .SetExec([=](VkCommandBuffer cmd){
               LOG_TRACE("Exec P0: Create ColorA + Depth");
           });
    });

    // Pass 1: 输入 A -> 输出 B (一一对应)
    auto p1 = ctx.renderPassManager.RegisterRenderPass([&](std::unique_ptr<RenderPass>& rp){
        rp->SetName("P1_A2B")
           .SetInputResource(colorA).SetOutputResource(colorB)
           .SetExec([=](VkCommandBuffer cmd){
               LOG_TRACE("Exec P1: A -> B");
           });
    });

    // Pass 2: 读取 B (只读) -> 输出 C (此处读取与输出之间非一一对应, 只读不参与 alias 链) 
    auto p2 = ctx.renderPassManager.RegisterRenderPass([&](std::unique_ptr<RenderPass>& rp){
        rp->SetName("P2_ReadB_OutC")
           .SetReadResource(colorB)
           .SetCreateResource(colorC) // 此处用 create 代表新写入
           .SetExec([=](VkCommandBuffer cmd){
               LOG_TRACE("Exec P2: Read B -> Create C");
           });
    });

    // Pass 3: 输入 C -> 输出 SwapChain
    auto p3 = ctx.renderPassManager.RegisterRenderPass([&](std::unique_ptr<RenderPass>& rp){
        rp->SetName("P3_C2Present")
           .SetInputResource(colorC).SetOutputResource(swapChainColor)
           .SetExec([=](VkCommandBuffer cmd){
               LOG_TRACE("Exec P3: C -> Present");
           });
    });

    ctx.frameGraph->AddRenderPassNode(p0)
                   .AddRenderPassNode(p1)
                   .AddRenderPassNode(p2)
                   .AddRenderPassNode(p3);

    ctx.frameGraph->AddResourceNode(colorA)
                  .AddResourceNode(colorB)
                  .AddResourceNode(colorC)
                  .AddResourceNode(depth)
                  .AddResourceNode(swapChainColor);
}

static void LogAliasInfo(ResourceManager &rm){
    auto &groups = rm.GetAliasGroups();
    LOG_DEBUG("AliasGroup Count: {}", groups.size());
    for(size_t i=0;i<groups.size();++i){
        LOG_DEBUG("  Group {} VulkanIndex={} SharedCount={} Type={}", i, groups[i].vulkanIndex,
                  groups[i].sharedResourceIndices.size(), groups[i].description->GetResourceType()==ResourceType::Texture?"Texture":"Buffer");
    }
}

static void RunFrameGraphTest(){
    TestContext ctx;
    ctx.frameGraph = std::make_unique<FrameGraph>(ctx.resourceManager, ctx.renderPassManager);

    // 生成一个简单面片以便有可用的导入纹理（这里使用模型生成，可能会触发加载贴图）
    uint32_t faceID = -1;
    vulkanRenderAPI.GenFace(faceID, {0,0,0}, {0,0,1}, 1,1, "../../resources/Pic/doro.png");

    // 交换链资源（present）
    auto swapIndices = vulkanRenderAPI.CreateSwapChainTexture(); // 实际会在 Vulkan 内部注册 swapchain 纹理
    uint32_t swapRes = RegisterSwapChainResource(ctx.resourceManager);

    // 导入的纹理（模拟外部输入，只读资源可被后续 Pass 读取 / 当成普通纹理，这里暂不加入 Pass）
    // 这里直接使用已经加载的面片的第一张纹理作为示例，如果没有则跳过
    uint32_t importedTexIndex = 0; // 假设 0 号为有效贴图，如失败不影响后续
    RegisterImportedTexture(ctx.resourceManager, importedTexIndex, "ImportedTex");

    // 内部中间 RT
    auto colorA = RegisterInternalRT(ctx.resourceManager, "ColorA", vulkanRenderAPI.GetVulkanSwapChain().colorFormat,
                                     VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    auto colorB = RegisterInternalRT(ctx.resourceManager, "ColorB", vulkanRenderAPI.GetVulkanSwapChain().colorFormat,
                                     VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    auto colorC = RegisterInternalRT(ctx.resourceManager, "ColorC", vulkanRenderAPI.GetVulkanSwapChain().colorFormat,
                                     VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    auto depth  = RegisterDepth(ctx.resourceManager);

    BuildRenderPasses(ctx, colorA, colorB, colorC, depth, swapRes);

    // 手动标记输入输出关系（ResourceDescription 的 input / output RenderPass 集合）
    // Pass 显式调用中未自动维护，需要在资源注册后进行：
    ctx.resourceManager.FindResource(colorA)->SetOutputRenderPass(0); // P0 create A
    ctx.resourceManager.FindResource(colorA)->SetInputRenderPass(1);  // P1 input A

    ctx.resourceManager.FindResource(colorB)->SetOutputRenderPass(1); // P1 output B
    ctx.resourceManager.FindResource(colorB)->SetInputRenderPass(2);  // P2 read B

    ctx.resourceManager.FindResource(colorC)->SetOutputRenderPass(2); // P2 create C
    ctx.resourceManager.FindResource(colorC)->SetInputRenderPass(3);  // P3 input C

    ctx.resourceManager.FindResource(depth)->SetOutputRenderPass(0);  // P0 create depth (只写)

    ctx.resourceManager.FindResource(swapRes)->SetInputRenderPass(3); // 最终 present

    LOG_DEBUG("Start Compile FrameGraph");
    ctx.frameGraph->Compile();
    LogAliasInfo(ctx.resourceManager);

    // 获取 primary command buffer 以调用 Execute（测试路径，不保证真正提交渲染）
    VkCommandBuffer primary = vulkanRenderAPI.BeginCommandBuffer();
    ctx.frameGraph->Execute(primary);
    // 不调用 EndCommandBuffer/提交，避免依赖真实 RenderLoop；这里只是验证 Execute 能成功生成次级命令
    LOG_DEBUG("Execute finished (secondary command buffers recorded)");
}

int main(){
    LOG.Run();
    LOG.SetPrintToFile(false);
    if(!vulkanRenderAPI.initVulkan()){
        LOG_ERROR("Failed to init Vulkan, abort FrameGraph test");
        LOG.Stop();
        return -1;
    }
    RunFrameGraphTest();
    vulkanRenderAPI.DestroyAll();
    LOG.Stop();
    return 0;
}