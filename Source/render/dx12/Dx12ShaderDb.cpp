#include "../Config.h"

#if ENABLE_DX12

#include <coalpy.files/Utils.h>
#include <coalpy.core/Assert.h>
#include "Dx12ShaderDb.h" 
#include <string>

#include <windows.h>
#include <dxc/inc/dxcapi.h>

namespace coalpy
{

namespace
{

const char* g_dxCompiler = "dxcompiler.dll";
HMODULE g_dxcModule = nullptr;
DxcCreateInstanceProc g_dxcCreateInstanceFn = nullptr;

}

Dx12ShaderDb::Dx12ShaderDb(const ShaderDbDesc& desc)
: m_compiler(nullptr), m_desc(desc)
{
    SetupDxc();
}

void Dx12ShaderDb::SetupDxc()
{
    if (g_dxcModule == nullptr)
    {
        std::string moduleName ="dxcompiler.dll";
        if (m_desc.compilerDllPath != nullptr)
            moduleName = std::string(m_desc.compilerDllPath) + "\\" + moduleName;

        std::string fullModulePath;
        FileUtils::getAbsolutePath(moduleName, fullModulePath);
        g_dxcModule = LoadLibraryA(fullModulePath.c_str());
        CPY_ASSERT_MSG(g_dxcModule != nullptr, "Error: could not load dxc compiler. Missing dll.");

        if (g_dxcModule)
        {
            g_dxcCreateInstanceFn = (DxcCreateInstanceProc)GetProcAddress(g_dxcModule, "DxcCreateInstance");
            CPY_ASSERT_MSG(g_dxcCreateInstanceFn, "Could not find \"DxcCreateInstance\" inside dxcompiler.dll");
        }
    }

    if (!g_dxcCreateInstanceFn)
        return;

    DX_OK(g_dxcCreateInstanceFn(CLSID_DxcCompiler, __uuidof(IDxcCompiler3), (void**)&m_compiler));
}

Dx12ShaderDb::~Dx12ShaderDb()
{
    if (m_compiler)
        m_compiler->Release();
}

void Dx12ShaderDb::compileShader(const ShaderDesc& desc, ShaderCompilationResult& result)
{
}

IShaderDb* IShaderDb::create(const ShaderDbDesc& desc)
{
    return new Dx12ShaderDb(desc);
}

}

#endif
