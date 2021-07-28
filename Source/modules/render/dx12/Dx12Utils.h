#pragma once
#include <d3d12.h>

namespace coalpy
{
namespace render
{

template<class T>
T alignByte(T offset, T alignment)
{
	return (offset + (alignment - 1)) & ~(alignment - 1);
}

}
}

typedef ID3D12GraphicsCommandList5 ID3D12GraphicsCommandListX;
