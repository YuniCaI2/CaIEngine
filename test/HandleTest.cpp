//
// Created by 51092 on 2025/11/18.
//
#include<vulkanFrameWork.h>
#include"Logger.h"


int main() {
	LOG.Run();
	auto textureHandle = vulkanRenderAPI.CreateResource<FrameWork::Texture>();
	auto *ptr = vulkanRenderAPI.GetResource(textureHandle);
    auto meshHandle = vulkanRenderAPI.CreateResource<FrameWork::Mesh>();
    auto *meshPtr = vulkanRenderAPI.GetResource(meshHandle);
    meshPtr->indexCount = 100;
    meshPtr->vertexCount = 200;
    LOG_TRACE("Index Count : {}, Vertex Count : {}", meshPtr->indexCount, meshPtr->vertexCount);
    auto meshHandleRef = vulkanRenderAPI.CopyResource(meshHandle);
    auto meshRefPtr = vulkanRenderAPI.GetResource(meshHandleRef);
    if(meshRefPtr == meshPtr) {
        LOG_TRACE("Copy Resource Success !");
    } else {
        LOG_ERROR("Copy Resource Failed !");
    }
    LOG_TRACE("Index Count : {}, Vertex Count : {}", meshRefPtr->indexCount, meshRefPtr->vertexCount);

    

    ptr->inUse = true;
    vulkanRenderAPI.DeleteResource(textureHandle);
    vulkanRenderAPI.DeleteResource(meshHandle);
    vulkanRenderAPI.DeleteResource(meshHandleRef);


    //保证资源释放
    vulkanRenderAPI.CheckDelete();
    vulkanRenderAPI.CheckDelete();
    vulkanRenderAPI.CheckDelete();

    auto newTextureHandle = vulkanRenderAPI.CreateResource<FrameWork::Texture>();
    auto textureHandleRef = vulkanRenderAPI.CopyResource(textureHandle);

    LOG_TRACE("Index Count : {}, Vertex Count : {}", meshPtr->indexCount, meshPtr->vertexCount);
    //delete meshPtr;
    LOG_TRACE("Index Count : {}, Vertex Count : {}", meshPtr->indexCount, meshPtr->vertexCount);
	// auto firstMeshRef = vulkanRenderAPI.GetResource(newTextureHandle);
	LOG.Stop();

}
