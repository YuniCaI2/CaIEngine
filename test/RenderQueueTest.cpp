// test_render_queue_manager.cpp
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <iomanip>

// 如果你的工程已经在这些头里定义 Camera/枚举，请直接删掉本地的 Camera 定义
#include "RenderQueueManager.h"
#include "RenderQueue.h"
#include "PublicEnum.h"

// ====== 极简 Camera（若你已有正式 Camera，请删掉此定义，用你的）======

// =====================================================================

// 小工具：打印一个 RenderQueue（你的 RenderLists 是 passName -> vector<unique_ptr<DrawItem>>）
static void DumpQueue(FrameWork::RenderQueue* q, const char* title, bool showSortKey = true) {
    using RenderLists = decltype(q->GetRenderLists());
    const RenderLists& lists = q->GetRenderLists();

    std::cout << "\n==== " << title << " ====\n";
    for (const auto& [pass, vecItems] : lists) {
        std::cout << "[Pass] " << pass << "  (count=" << vecItems.size() << ")\n";
        int i = 0;
        for (const auto& up : vecItems) {
            const auto& d = *up;
            std::cout << "  #" << std::setw(2) << i++
                      << " mesh="      << d.meshID
                      << " pipe="      << d.pipelineID
                      << " mat="       << d.materialID
                      << " pos=("      << d.position.x << "," << d.position.y << "," << d.position.z << ")"
                      << " depthQ="    << d.depth;
            if (showSortKey) std::cout << " key=" << d.sortKey;
            std::cout << "\n";
        }
    }
}

int main() {
    using namespace FrameWork;

    // 1) 相机
    Camera cam;
    cam.Position = {0, 0, 0};
    cam.Front    = {0, 0, -1};  // 看向 -Z

    // 2) 取 RenderQueueManager 单例
    auto& mgr = RenderQueueManager::GetInstance();

    // 3) 构造若干 Opaque 项（不同 passName / pipeline / material / 距离）
    {
        auto d = std::make_unique<DrawItem>();
        d->passName   = "GBuffer";
        d->meshID     = 1;
        d->pipelineID = 10;
        d->materialID = 100;
        d->position   = {0, 0, -5};   // 近
        mgr.AddDrawItem(std::move(d), RenderQueueType::Opaque);
    }
    {
        auto d = std::make_unique<DrawItem>();
        d->passName   = "GBuffer";
        d->meshID     = 2;
        d->pipelineID = 10;           // 同管线，期望相邻
        d->materialID = 101;
        d->position   = {0, 0, -20};  // 远
        mgr.AddDrawItem(std::move(d), RenderQueueType::Opaque);
    }
    {
        auto d = std::make_unique<DrawItem>();
        d->passName   = "Shadow";     // 不同 pass
        d->meshID     = 3;
        d->pipelineID = 5;
        d->materialID = 50;
        d->position   = {10, 0, -8};
        mgr.AddDrawItem(std::move(d), RenderQueueType::Opaque);
    }

    // 4) 构造若干 Transparent 项（要验证 B2F：远→近）
    {
        auto d = std::make_unique<DrawItem>();
        d->passName   = "TransparentComposite";
        d->meshID     = 4;
        d->pipelineID = 30;
        d->materialID = 300;
        d->position   = {0, 0, -50}; // 最远
        mgr.AddDrawItem(std::move(d), RenderQueueType::Transparent);
    }
    {
        auto d = std::make_unique<DrawItem>();
        d->passName   = "TransparentComposite";
        d->meshID     = 5;
        d->pipelineID = 31;           // 不同管线
        d->materialID = 301;
        d->position   = {0, 0, -10};  // 最近
        mgr.AddDrawItem(std::move(d), RenderQueueType::Transparent);
    }
    {
        auto d = std::make_unique<DrawItem>();
        d->passName   = "TransparentComposite";
        d->meshID     = 6;
        d->pipelineID = 30;
        d->materialID = 302;
        d->position   = {0, 0, -25}; // 中等远
        mgr.AddDrawItem(std::move(d), RenderQueueType::Transparent);
    }

    // 5) 排序前打印（看原始插入顺序）
    std::cout << "=== BEFORE SORT ===\n";
    DumpQueue(mgr.GetRenderQueue(RenderQueueType::Opaque),      "Opaque Queue (raw)");
    DumpQueue(mgr.GetRenderQueue(RenderQueueType::Transparent), "Transparent Queue (raw)");

    // 6) 触发排序（内部会对每个 pass 的 vector 调用 MakeSortKey + sort）
    //    你可以根据队列类型选择 F2B/B2F，这里演示分别调用两次（也可以你的实现里内部决定）
    mgr.SortAll(cam);

    // 7) 排序后打印（检查是否符合预期）
    std::cout << "\n=== AFTER SORT ===\n";
    DumpQueue(mgr.GetRenderQueue(RenderQueueType::Opaque),      "Opaque Queue (sorted)");
    DumpQueue(mgr.GetRenderQueue(RenderQueueType::Transparent), "Transparent Queue (sorted)");

    // 8) 清理
    mgr.ClearAll();

    std::cout << "\nTest done.\n";
    return 0;
}
