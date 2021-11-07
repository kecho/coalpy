#pragma once

#include <Windows.h>
#include <D3D12.h>

namespace coalpy
{
namespace render
{

class Dx12PixApi
{
public:
    typedef HRESULT(WINAPI* BeginEventOnCommandList)(ID3D12GraphicsCommandList* commandList, UINT64 color, _In_ PCSTR formatString);
    typedef HRESULT(WINAPI* EndEventOnCommandList)(ID3D12GraphicsCommandList* commandList);
    typedef HRESULT(WINAPI* SetMarkerOnCommandList)(ID3D12GraphicsCommandList* commandList, UINT64 color, _In_ PCSTR formatString);

    BeginEventOnCommandList pixBeginEventOnCommandList = nullptr;
    EndEventOnCommandList   pixEndEventOnCommandList   = nullptr;
    SetMarkerOnCommandList  pixSetMarkerOnCommandList  = nullptr;

    static Dx12PixApi* get(const char* resourcePath); 
};

}
}
