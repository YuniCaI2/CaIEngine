#pragma once

namespace CaIEngine {
    enum class FrameBufferType {
        Present,
        PresentOverspread,
        // 这里两个Present类型是为了解决Vulkan的问题，在非编辑器模式下，UI是直接写入Present Buffer的
		// AfterEffectPass写入Present Buffer的时候直接覆盖即可，但是UI写入的时候需要保留Present Buffer的内容
		// 所以有两种写入方式，PresentOverspread是直接覆盖写入，Present是写入时保留原Present Buffer的内容
        Normal,
        HigthPrecision,
        Color,
        ShadowMap,
        ShadowCubeMap,
        GBuffer,
        Deferred,
        RayTracing
    };
} // namespace CaIEngine