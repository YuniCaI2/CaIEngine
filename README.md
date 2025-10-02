# CaIEngine 介绍

## Vulkan封装

- 使用vulkanFrameWork类对Vulkan繁琐API进行封装，并且使用池统一管理Vulkan对象，实现延迟释放保证多飞行帧的数据安全
- 集成ImGUI，并封装，支持调试时快速调用
- 使用ShaderParse对GLSL进行简易包装，并且附带Vulkan Pipeline信息构建，代替繁琐的Vulkan管线构建的过程，且使用cache记录编译时间以避免重复编译
  以下是我着色器的格式实例，通过ShaderParse解析成GLSL语言并且通过GLSLANG自动化编译成.spv

  ```
  Settings {
      Blend SrcAlpha OneMinusSrcAlpha
      BlendOp Add
      Cull None
      ZTest LessOrEqual
      ZWrite Off
      InputVertex Off
  }
  
  Vertex{
      Output{
          0 vec2 fragTexCoord
      }
      Program{
      vec2 positions[6] = vec2[](
      vec2(0.0, 0.0),  // 左下
      vec2(1.0, 0.0),  // 右下
      vec2(1.0, 1.0),  // 右上
      vec2(0.0, 0.0),  // 左下
      vec2(1.0, 1.0),  // 右上
      vec2(0.0, 1.0)   // 左上
      );
  
      // 对应的纹理坐标
      vec2 texCoords[6] = vec2[](
      vec2(0.0, 0.0),  // 左下
      vec2(1.0, 0.0),  // 右下
      vec2(1.0, 1.0),  // 右上
      vec2(0.0, 0.0),  // 左下
      vec2(1.0, 1.0),  // 右上
      vec2(0.0, 1.0)   // 左上
      );
  
  
  
          void main(){
              // 获取当前顶点索引
              int vertexIndex = gl_VertexIndex;
  
              // 获取顶点位置（范围 [0,1]）
              vec2 pos = positions[vertexIndex];
  
              // 转换到NDC坐标系 (范围 [-1,1])
              vec2 ndcPos = pos * 2.0 - 1.0;
  
              // 设置输出位置
              gl_Position = vec4(ndcPos, 0.0, 1.0);
  
              // 传递纹理坐标到片段着色器
              fragTexCoord = texCoords[vertexIndex];
  
          }
      }
  }
  
  Fragment{
      Input{
          0 vec2 fragTexCoord
      }
      Output{
          0 vec4 fragColor
      }
      Properties{
          sampler2D colorTexture
      }
      Program{
          void main() {
              // 采样上一个pass的颜色
              fragColor = texture(colorTexture, fragTexCoord);
               // fragColor = vec4(1.0, 0.0, 0.0, 1.0);
          }
      }
  }
  ```



## FrameGraph 系统

- 自动分析 Pass 之间的依赖关系，支持 **多线程并行录制 CommandBuffer**

- 支持 **Graphics 与 Compute Shader Pass**，统一调度并管理跨队列同步

- 提供 **资源延迟释放机制**，保证跨帧资源的安全管理与复用

  以下是简单的FrameGraph示例：

```cpp
    //创建持久资源
    std::string shaderPath = "../resources/CaIShaders/BaseScene/BaseScene.caishader";
    std::string testShaderPath = "../resources/CaIShaders/TestFrameGraph/forward.caishader";

    FrameWork::CaIShader::Create(caiShaderID, shaderPath);
    FrameWork::CaIShader::Create(testShader, testShaderPath);
    vulkanRenderAPI.LoadVulkanModel(vulkanModelID, "cocona", ModelType::OBJ, DiffuseColor, {0,0, 0}, 1.0f);
    auto model = vulkanRenderAPI.getByIndex<FrameWork::VulkanModelData>(vulkanModelID);
    // 绑定静态纹理
     materials.resize(model->meshIDs.size());
     for (int i = 0; i < model->meshIDs.size(); i++) {
         FrameWork::CaIMaterial::Create(materials[i], caiShaderID);
         FrameWork::CaIMaterial::Get(materials[i])->SetTexture("colorSampler", model->textures[i][DiffuseColor]);
     }
    std::string presentShaderPath = "../resources/CaIShaders/Present/Present.caishader";
    FrameWork::CaIShader::Create(presentShaderID, presentShaderPath);
    FrameWork::CaIMaterial::Create(presentMaterialID, presentShaderID);

    //创建FrameGraph资源
    colorAttachment = resourceManager.RegisterResource(
        [&](std::unique_ptr<FG::ResourceDescription>& desc) {
            desc->SetName("ColorAttachment")
            .SetDescription<FG::TextureDescription>(
                std::make_unique<FG::TextureDescription>(
                    vulkanRenderAPI.GetFrameWidth(), vulkanRenderAPI.GetFrameHeight(),
                    vulkanRenderAPI.GetVulkanSwapChain().colorFormat, 1, 1, vulkanRenderAPI.GetSampleCount(),
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
                    )
                );
        }
        );

    depthAttachment = resourceManager.RegisterResource(
        [&](std::unique_ptr<FG::ResourceDescription>& desc) {
            desc->SetName("DepthAttachment")
            .SetDescription<FG::TextureDescription>(
                std::make_unique<FG::TextureDescription>(
                    vulkanRenderAPI.GetFrameWidth(), vulkanRenderAPI.GetFrameHeight(),
                    vulkanRenderAPI.GetDepthFormat() , 1, 1, vulkanRenderAPI.GetSampleCount(),
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
                    )
                );
        }
        );

    swapChainAttachment = resourceManager.RegisterResource([&](std::unique_ptr<FG::ResourceDescription> &desc) {
        desc->SetName("swapChain");
        desc->SetDescription<FG::TextureDescription>(std::make_unique<FG::TextureDescription>());
        desc->isExternal = true;
        desc->isPresent = true;
        desc->vulkanIndex = vulkanRenderAPI.GetSwapChainTextures()[0]; //先随便绑定一个
    });

    auto forwardPass = renderPassManager.RegisterRenderPass([this](std::unique_ptr<FG::RenderPass> &renderPass) {
    renderPass->SetName("forwardPass");
    renderPass->SetExec([&](VkCommandBuffer cmdBuffer) {
            FrameWork::CaIShader::Get(caiShaderID)->Bind(cmdBuffer);
            auto model = vulkanRenderAPI.getByIndex<FrameWork::VulkanModelData>(vulkanModelID);
            glm::mat4 projection = glm::perspective(glm::radians(cameraPtr->Zoom),
                                  (float) vulkanRenderAPI.windowWidth / (float) vulkanRenderAPI.windowHeight,
                                  0.01f, 100.0f);
            projection[1][1] *= -1;
            glm::mat4 pos = glm::translate(glm::mat4(1.0), model->position);
            for (int i = 0; i < model->meshIDs.size(); i++) {
                auto material = FrameWork::CaIMaterial::Get(materials[i]);
                material->SetParam("viewMatrix", cameraPtr->GetViewMatrix(), 0);
                material->SetParam("projectionMatrix", projection, 0);
                material->SetParam("modelMatrix", pos, 0);
                material->Bind(cmdBuffer);
                VkDeviceSize offsets[] = {0};
                auto mesh = vulkanRenderAPI.getByIndex<FrameWork::Mesh>(model->meshIDs[i]);
                vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &mesh->VertexBuffer.buffer, offsets);
                vkCmdBindIndexBuffer(cmdBuffer, mesh->IndexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(cmdBuffer, mesh->indexCount, 1, 0, 0, 0); //没有进行实例渲染
            }

        });
    });

    auto presentPass = renderPassManager.RegisterRenderPass([this](auto &renderPass) {
        renderPass->SetName("presentPass");
        renderPass->SetExec([&](VkCommandBuffer cmdBuffer) {
            //绑定对应imageView
            FrameWork::CaIShader::Get(presentShaderID)->Bind(cmdBuffer);
            FrameWork::CaIMaterial::Get(presentMaterialID)->SetTexture("colorTexture", resourceManager.GetVulkanResolveIndex(colorAttachment));
            FrameWork::CaIMaterial::Get(presentMaterialID)->Bind(cmdBuffer);
            vkCmdDraw(cmdBuffer, 6, 1, 0, 0);
        });
    });

    //构建图
    frameGraph->AddResourceNode(colorAttachment).AddResourceNode(swapChainAttachment).AddResourceNode(depthAttachment)
            .AddRenderPassNode(forwardPass).AddRenderPassNode(presentPass);

    //设置每帧资源更新
    frameGraph->SetUpdateBeforeRendering([this]() {
        auto swapChainDesc = resourceManager.FindResource(swapChainAttachment);
        swapChainDesc->vulkanIndex = vulkanRenderAPI.GetSwapChainTextures()[vulkanRenderAPI.GetCurrentImageIndex()];
        //防止窗口更新不对齐
        swapChainDesc->GetDescription<FG::TextureDescription>()->width = vulkanRenderAPI.windowWidth;
        swapChainDesc->GetDescription<FG::TextureDescription>()->height = vulkanRenderAPI.windowHeight;

        auto colorAttachDesc = resourceManager.FindResource(colorAttachment);
        auto depthAttachDesc = resourceManager.FindResource(depthAttachment);


    });

    renderPassManager.FindRenderPass(forwardPass)->SetCreateResource(colorAttachment).SetCreateResource(depthAttachment);
    renderPassManager.FindRenderPass(presentPass)->SetCreateResource(swapChainAttachment).SetReadResource(
        colorAttachment);

    frameGraph->Compile();
```

渲染则调用`frameGraph->Execute()` 即可


## 场景实现
	•	LTC 光照 —— 真实感区域光模拟
	•	PCSS 阴影 —— 基于采样的软阴影算法
	•	Bloom 后处理 —— 高光泛光特效

  以下是LTC场景的展示

  <img width="1275" height="716" alt="image" src="https://github.com/user-attachments/assets/b34611ca-60a6-4478-a27a-c26e47100517" />

