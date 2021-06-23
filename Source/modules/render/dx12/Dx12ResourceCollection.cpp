#pragma once

#include <Config.h>

#if ENABLE_DX12

#include "Dx12ResourceCollection.h"
#include "Dx12Device.h"
#include <coalpy.core/Assert.h>

namespace coalpy
{
namespace render
{

Dx12ResourceCollection::Dx12ResourceCollection(Dx12Device& device)
: m_device(device)
{
}

Dx12ResourceCollection::~Dx12ResourceCollection()
{
}

Texture Dx12ResourceCollection::createTexture(const TextureDesc& desc)
{
    std::unique_lock lock(m_resourceMutex);
    ResourceHandle resHandle;
    auto& outPtr = m_resources.allocate(resHandle);
    if (outPtr == nullptr)
        outPtr = new ResourceContainer();
    CPY_ASSERT(outPtr->resource == nullptr);
    
    outPtr->type = ResType::Texture;
    outPtr->resource = new Dx12Texture(m_device, desc);
    return Texture { resHandle.handleId };
}

Buffer  Dx12ResourceCollection::createBuffer (const BufferDesc& desc)
{
    std::unique_lock lock(m_resourceMutex);
    ResourceHandle resHandle;
    auto& outPtr = m_resources.allocate(resHandle);
    if (outPtr == nullptr)
        outPtr = new ResourceContainer();
    CPY_ASSERT(outPtr->resource == nullptr);
    
    outPtr->type = ResType::Buffer;
    outPtr->resource = new Dx12Buffer(m_device, desc);
    return Buffer { resHandle.handleId };
}

void Dx12ResourceCollection::release(ResourceHandle handle)
{
    std::unique_lock lock(m_resourceMutex);
    bool isValid = handle.valid() && m_resources.contains(handle);
    CPY_ASSERT(isValid);
    if (!isValid)
        return;
    (*m_resources[handle]) = {};
    m_resources.free(handle, false);
}

}
}

#endif
