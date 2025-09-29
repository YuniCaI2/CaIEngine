//
// Created by 51092 on 2025/9/29.
//

#ifndef CAIENGINE_UNIFORMPASS_H
#define CAIENGINE_UNIFORMPASS_H
#include "../FrameGraph.h"

//对通用Pass的封装方便调用，当然这也不一定是一个节点，可能是一串（比如降采样）
namespace FG {
    class UniformPass {
    public:
        virtual ~UniformPass() = default;
        UniformPass() =default;
        virtual void Bind() = 0;
        virtual void SetInputOutputResource(const uint32_t& index0, uint32_t& index1) = 0;
        virtual void SetCreateResource(uint32_t& index) = 0;
        virtual void SetReadResource(const uint32_t& index) = 0;

    };
}


#endif //CAIENGINE_UNIFORMPASS_H