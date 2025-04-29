#pragma once

namespace CaIEngine {
    enum class FrameBufferType {
        Present,
        PresentOverspread,
        // 这里两个Present类型是为了解决Vulkan的问题，在非编辑器模式下，UI是直接写入Present Buffer的
		// AfterEffectPass写入Present Buffer的时候直接覆盖即可，但是UI写入的时候需要保留Present Buffer的内容
		// 所以有两种写入方式，PresentOverspread是直接覆盖写入，Present是写入时保留原Present Buffer的内容
        Normal,//普通的标准的渲染过程
        HigthPrecision,
        Color,
        ShadowMap,
        ShadowCubeMap,
        GBuffer,
        Deferred,
        RayTracing
    };

    enum class CommandType {
        NotCare,// /不关心的命令，只是占位符
        ShadowGeneration,
        ForwardRendering,
        GBufferRendering,//几何缓冲
        DeferredRendering,//延迟渲染的着色和光照计算
        AfterEffectRendering,
        UIRendering,
        AssetPreviewer,
        RayTracing,
        Compute
    };

    using FrameBufferClearFlags = uint32_t;
    enum FrameBufferClearFlagBit {
        CAI_CLEAR_FRAME_BUFFER_NONE_BIT = 0x00000000,
        CAI_CLEAR_FRAME_BUFFER_COLOR_BIT = 0x00000001,
        CAI_CLEAR_FRAME_BUFFER_DEPTH_BIT = 0x00000002,
        CAI_CLEAR_FRAME_BUFFER_STENCIL_BIT = 0x00000004,
    };

    enum class GPUBufferType{
        Static,
        DynamicCPUWriteGPURead,
        DynamicGPUWriteCPURead,//CPU可以写入GPU，GPU也可以读取
        DynamicGPUWriteGPURead,
    };



} // namespace CaIEngine