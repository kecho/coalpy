#include <Config.h>
#include "Dx12Compiler.h"
#if ENABLE_DX12

#include <coalpy.core/Assert.h>
#include <coalpy.core/GenericHandle.h>
#include <coalpy.core/HandleContainer.h>
#include <coalpy.core/SmartPtr.h>
#include <coalpy.core/String.h>
#include <coalpy.files/Utils.h>

#include <string>
#include <vector>
#include <windows.h>
#include <dxc/inc/dxcapi.h>

namespace coalpy
{

namespace 
{

const char* g_dxCompiler = "dxcompiler.dll";
HMODULE g_dxcModule = nullptr;
DxcCreateInstanceProc g_dxcCreateInstanceFn = nullptr;

struct DxcCompilerHandle : public GenericHandle<unsigned int> {};
struct DxcUtilsHandle : public GenericHandle<unsigned int>{};

struct DxcInstance
{
    DxcCompilerHandle compiler;
    DxcUtilsHandle utils;
};

struct DxcInstanceData
{
    IDxcCompiler3& compiler;
    IDxcUtils& utils;
};

class DxcPool
{
public:
    DxcInstance allocate()
    {
        DxcInstance h;

        {
            SmartPtr<IDxcCompiler3>& instance = m_compilerPool.allocate(h.compiler);
            if (instance == nullptr)
                DX_OK(g_dxcCreateInstanceFn(CLSID_DxcCompiler, __uuidof(IDxcCompiler3), (void**)&instance));
        }

        {
            SmartPtr<IDxcUtils>& instance = m_utilsPool.allocate(h.utils);
            if (instance == nullptr)
                DX_OK(g_dxcCreateInstanceFn(CLSID_DxcUtils, __uuidof(IDxcUtils), (void**)&instance));
        }

        return h;
    }

    DxcInstanceData operator [](DxcInstance instance)
    {
        CPY_ERROR(m_compilerPool.contains(instance.compiler));
        CPY_ERROR(m_utilsPool.contains(instance.utils));
        SmartPtr<IDxcCompiler3>& c = m_compilerPool[instance.compiler];
        SmartPtr<IDxcUtils>& u = m_utilsPool[instance.utils];
        return DxcInstanceData { *c, *u };
    }

    void release(DxcInstance instance)
    {
        m_compilerPool.free(instance.compiler, false/*dont reset object so we recycle it*/);
        m_utilsPool.free(instance.utils, false/*dont reset object so we recycle it*/);
    }

private:
    HandleContainer<DxcCompilerHandle, SmartPtr<IDxcCompiler3>> m_compilerPool;
    HandleContainer<DxcUtilsHandle, SmartPtr<IDxcUtils>> m_utilsPool;
};

thread_local DxcPool s_dxcPool;

struct DxcCompilerScope
{
    DxcCompilerScope()
    {
        m_instance = s_dxcPool.allocate();
    }

    ~DxcCompilerScope()
    {
        s_dxcPool.release(m_instance);
    }

    inline DxcInstanceData data() { return s_dxcPool[m_instance]; }
    DxcInstance m_instance;
};

}

Dx12Compiler::Dx12Compiler(const ShaderDbDesc& desc)
: m_desc(desc)
{
}

Dx12Compiler::~Dx12Compiler()
{
}

void Dx12Compiler::compileShader(
        ShaderType type,
        const char* mainFn,
        const char* source,
        const std::vector<std::string>& defines,
        ShaderCompilationResult& result)
{
    DxcCompilerScope scope;
    DxcInstanceData instanceData = scope.data();
    IDxcCompiler3& compiler = instanceData.compiler;
    IDxcUtils& utils = instanceData.utils;

    std::vector<LPWSTR>  arguments;
    std::vector<std::wstring> argumentsMem;

    argumentsMem.push_back(L"-E");
    argumentsMem.push_back(s2ws(std::string(mainFn)));

    //latest and greatest shader model 6.6 baby!!
    static const wchar_t* s_targets[ShaderType::Count] = {
        L"vs_6_6", //Vertex
        L"ps_6_6", //Pixel
        L"cs_6_6"  //Compute
    };
    
    argumentsMem.push_back(L"-T");
    argumentsMem.push_back(s_targets[(int)type]);

    for (auto& defStr : defines)
    {
        argumentsMem.push_back(L"-D");
        argumentsMem.push_back(s2ws(defStr));
    }

    
}

void Dx12Compiler::setupDxc()
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

    //DX_OK(g_dxcCreateInstanceFn(CLSID_DxcCompiler, __uuidof(IDxcCompiler3), (void**)&m_compiler));
}


}

#endif
