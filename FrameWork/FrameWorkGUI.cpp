//
// Created by 51092 on 25-8-2.
//

#include "FrameWorkGUI.h"
#include <vector>
#include "VulkanTool.h"
#include "VulkanWindow.h"



void FrameWork::FrameWorkGUI::ReleaseGUIResources() const {
    vkDeviceWaitIdle(device);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    ImGui::DestroyContext();
    vkDestroyRenderPass(device, renderPass, nullptr);
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    vkDestroyPipelineCache(device, pipelineCache, nullptr);
    for (auto &f: framebuffers) {
        vkDestroyFramebuffer(device, f, nullptr);
    }
}


void FrameWork::FrameWorkGUI::InitFrameWorkGUI() {
    device = vulkanRenderAPI.vulkanDevice->logicalDevice;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
                        ImGuiCond_FirstUseEver,
                        ImVec2(0.0f, 0.0f)); // pivot点设为(0.5, 0.5)表示中心点
    ImGui::SetNextWindowSize(ImVec2(100, 100), ImGuiCond_FirstUseEver);

    // 设置样式
    ImGui::StyleColorsDark();


    //生成Vulkan资源

    //DescriptorPool
    std::vector<VkDescriptorPoolSize> poolSizes = {
        {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}
    };

    VkDescriptorPoolCreateInfo descriptorPoolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = 1000,
        .poolSizeCount = (uint32_t) poolSizes.size(),
        .pPoolSizes = poolSizes.data()
    };

    VK_CHECK_RESULT(
        vkCreateDescriptorPool(
            vulkanRenderAPI.vulkanDevice->logicalDevice, &descriptorPoolInfo, nullptr, &descriptorPool
        )
    );

    //RenderPass
    VkAttachmentDescription colorAttachmentDescription = {
        .flags = 0,
        .format = vulkanRenderAPI.GetVulkanSwapChain().colorFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    VkAttachmentReference colorAttachmentReference = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass = {
        .flags = 0,
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .inputAttachmentCount = 0,
        .pInputAttachments = nullptr,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentReference,
        .pResolveAttachments = nullptr,
        .pDepthStencilAttachment = nullptr,
        .preserveAttachmentCount = 0,
        .pPreserveAttachments = nullptr
    };

    VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    VkRenderPassCreateInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .attachmentCount = 1,
        .pAttachments = &colorAttachmentDescription,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    VK_CHECK_RESULT(
        vkCreateRenderPass(
            vulkanRenderAPI.vulkanDevice->logicalDevice, &renderPassInfo, nullptr,
            &renderPass
        ));

    //FrameBuffer
    VkFramebufferCreateInfo framebufferInfo = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .renderPass = renderPass,
        .attachmentCount = 1,
        .pAttachments = &vulkanRenderAPI.GetVulkanSwapChain().imageViews[0],
        .width = vulkanRenderAPI.GetFrameWidth(),
        .height = vulkanRenderAPI.GetFrameHeight(),
        .layers = 1
    };
    framebuffers.resize(vulkanRenderAPI.GetVulkanSwapChain().imageViews.size());
    for (int i = 0; i < framebuffers.size(); i++) {
        framebufferInfo.pAttachments = &vulkanRenderAPI.GetVulkanSwapChain().imageViews[i];
        VK_CHECK_RESULT(
            vkCreateFramebuffer(vulkanRenderAPI.vulkanDevice->logicalDevice, &framebufferInfo, nullptr, &framebuffers[i]
            ));
    }

    //PipelineCache
    VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .initialDataSize = 0,
        .pInitialData = nullptr,
    };

    VK_CHECK_RESULT(
        vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &pipelineCache)
    );

    ImGui_ImplVulkan_LoadFunctions(VK_API_VERSION_1_3 ,[](const char *function_name, void *vulkan_instance) {
      return vkGetInstanceProcAddr(*(reinterpret_cast<VkInstance *>(vulkan_instance)), function_name);
    }, &vulkanRenderAPI.GetVulkanInstance());

    if (!ImGui_ImplGlfw_InitForVulkan(VulkanWindow::GetInstance().GetWindow(), true)) {
        std::cerr << "ImGui GLFW Init is failed" << std::endl;
    }


    ImGui_ImplVulkan_InitInfo vulkanInitInfo = {
        .Instance = vulkanRenderAPI.GetVulkanInstance(),
        .PhysicalDevice = vulkanRenderAPI.GetVulkanPhysicalDevice(),
        .Device = vulkanRenderAPI.vulkanDevice->logicalDevice,
        .QueueFamily = vulkanRenderAPI.vulkanDevice->queueFamilyIndices.graphics,
        .Queue = vulkanRenderAPI.GetVulkanGraphicsQueue(),
        .DescriptorPool = descriptorPool,
        .RenderPass = renderPass,
        .MinImageCount = vulkanRenderAPI.GetVulkanSwapChain().minImageCount,
        .ImageCount = (uint32_t) vulkanRenderAPI.GetVulkanSwapChain().imageViews.size(),
        .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
        .PipelineCache = pipelineCache,
        .Subpass = 0,
        .DescriptorPoolSize = 0,
        .UseDynamicRendering = false,
        .PipelineRenderingCreateInfo = {},
        .Allocator = nullptr,
        .CheckVkResultFn = nullptr,
        .MinAllocationSize = 1024 * 1024
    };


    ImGui_ImplVulkan_Init(&vulkanInitInfo);
    ImGui_ImplVulkan_CreateFontsTexture();

    auto windowResizeCallback = [this]() {
        for (uint32_t i = 0; i < framebuffers.size(); i++) {
            vkDestroyFramebuffer(vulkanRenderAPI.vulkanDevice->logicalDevice, framebuffers[i], nullptr);
        }

        VkFramebufferCreateInfo framebufferInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .renderPass = renderPass,
            .attachmentCount = 1,
            .pAttachments = &vulkanRenderAPI.GetVulkanSwapChain().imageViews[0],
            .width = vulkanRenderAPI.GetFrameWidth(),
            .height = vulkanRenderAPI.GetFrameHeight(),
            .layers = 1
        };
        framebuffers.resize(vulkanRenderAPI.GetVulkanSwapChain().imageViews.size());
        for (int i = 0; i < framebuffers.size(); i++) {
            framebufferInfo.pAttachments = &vulkanRenderAPI.GetVulkanSwapChain().imageViews[i];
            VK_CHECK_RESULT(
                vkCreateFramebuffer(vulkanRenderAPI.vulkanDevice->logicalDevice, &framebufferInfo, nullptr, &framebuffers[i]
                ));
        }
    };
    vulkanRenderAPI.SetWindowResizedCallBack(windowResizeCallback);
}

void FrameWork::FrameWorkGUI::SetGUIItems(const std::function<void()> &Func) {
    ItemFunc = Func;
}

void FrameWork::FrameWorkGUI::SetGUIItems(std::function<void()> &&Func) {
    ItemFunc = std::move(Func);
}


void FrameWork::FrameWorkGUI::RenderGUI(VkCommandBuffer commandBuffer) const {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (ImGui::Begin("Setting")) {
        ItemFunc();
    }
    ImGui::End();

    VkClearValue clearValues[2];
    clearValues[0].color = vulkanRenderAPI.defaultClearColor;
    clearValues[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,
        .renderPass = renderPass,
        .framebuffer = framebuffers[vulkanRenderAPI.GetCurrentImageIndex()],
        .renderArea = {
            .offset = {0, 0},
            .extent = {vulkanRenderAPI.GetFrameWidth(), vulkanRenderAPI.GetFrameHeight()}
            },
        .clearValueCount = 2,
        .pClearValues = clearValues,
    };
    ImGui::Render();
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    auto drawData = ImGui::GetDrawData();
    if (drawData && drawData->TotalIdxCount > 0) {
        ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);
    }
    vkCmdEndRenderPass(commandBuffer);
}
