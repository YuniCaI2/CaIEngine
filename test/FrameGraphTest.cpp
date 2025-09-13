//
// Created by cai on 2025/9/13.
//
#include<FrameGraph/ResourceManager.h>

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

int main() {
    LOG.Run();
    TestResourceManager();
    LOG.Stop();
}