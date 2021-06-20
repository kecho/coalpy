#pragma once

#include <Config.h>

#if ENABLE_DX12

#include "Dx12ResourceCollection.h"
#include "Dx12Device.h"

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

}
}

#endif
