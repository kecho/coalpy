#include "Dx12PixApi.h"
#include <sstream>

namespace coalpy
{
namespace render
{

namespace
{

const char* g_defaultPixModulePath = "coalpy\\resources";
Dx12PixApi g_pixApi;
HMODULE g_pixModule = nullptr;

}

Dx12PixApi* Dx12PixApi::get(const char* resourcePath)
{
    if (g_pixModule == nullptr)
    {
        std::stringstream ss;
        ss << (resourcePath == nullptr ? g_defaultPixModulePath : resourcePath);
        ss << "\\" << "WinPixEventRuntime.dll";
        g_pixModule = LoadLibrary(ss.str().c_str());
        if (g_pixModule)
        {
            g_pixApi.pixBeginEventOnCommandList = (BeginEventOnCommandList)GetProcAddress(g_pixModule, "PIXBeginEventOnCommandList");
            g_pixApi.pixEndEventOnCommandList   = (EndEventOnCommandList)GetProcAddress(g_pixModule, "PIXEndEventOnCommandList");
            g_pixApi.pixSetMarkerOnCommandList  = (SetMarkerOnCommandList)GetProcAddress(g_pixModule, "PIXSetMarkerOnCommandList");
        }
    }

    return g_pixModule == nullptr ? nullptr : &g_pixApi;
}

}
}
