#include <Config.h>
#include "Dx12Compiler.h"
#if ENABLE_DX12

#include <coalpy.core/Assert.h>
#include <coalpy.core/GenericHandle.h>
#include <coalpy.core/HandleContainer.h>
#include <coalpy.core/SmartPtr.h>
#include <coalpy.core/String.h>
#include <coalpy.core/ClTokenizer.h>
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
    setupDxc();
}

Dx12Compiler::~Dx12Compiler()
{
}

void Dx12Compiler::compileShader(const Dx12CompileArgs& args)
{
    //latest and greatest shader model 6.6 baby!!
    static const wchar_t* s_targets[ShaderType::Count] = {
        L"vs_6_6", //Vertex
        L"ps_6_6", //Pixel
        L"cs_6_6"  //Compute
    };

    DxcCompilerScope scope;
    DxcInstanceData instanceData = scope.data();
    IDxcCompiler3& compiler = instanceData.compiler;
    IDxcUtils& utils = instanceData.utils;

    SmartPtr<IDxcBlobEncoding> codeBlob;
    DX_OK(utils.CreateBlob(args.source, strlen(args.source), CP_UTF8, (IDxcBlobEncoding**)&codeBlob));

    std::string  sshaderName = args.shaderName;
    std::wstring wshaderName = s2ws(sshaderName);

    std::string  sentryPoint = args.mainFn;
    std::wstring wentryPoint = s2ws(sentryPoint);

    const wchar_t* profile = s_targets[(int)args.type];

    std::vector<LPCWSTR> arguments;
    arguments.push_back(DXC_ARG_WARNINGS_ARE_ERRORS);
    arguments.push_back(DXC_ARG_OPTIMIZATION_LEVEL3);
    arguments.push_back(DXC_ARG_ENABLE_STRICTNESS);

    SmartPtr<IDxcCompilerArgs> compilerArgs;

    //extremely important to not resize these containers, so we preserve the same pointers
    std::vector<std::wstring> defineStrings((int)args.defines.size() * 2u);
    std::vector<DxcDefine> dxcDefines;
    dxcDefines.reserve(args.defines.size());
    {
        int nextDefine = 0;
        for (auto& str : args.defines)
        {
            auto splitArr = ClTokenizer::splitString(str, '=');
            DxcDefine dxcDef = {};
            defineStrings[nextDefine] = s2ws(splitArr[0]);
            dxcDef.Name = defineStrings[nextDefine].c_str();
            ++nextDefine;
            if (splitArr.size() >= 2)
            {
                defineStrings[nextDefine] = s2ws(splitArr[1]);
                dxcDef.Value = defineStrings[nextDefine].c_str();
                ++nextDefine;
            }

            dxcDefines.push_back(dxcDef);
        }
    }

    DX_OK(utils.BuildArguments(
        wshaderName.c_str(),
        wentryPoint.c_str(),
        profile,
        arguments.data(),
        (UINT32)arguments.size(),
        dxcDefines.data(),
        (UINT32)dxcDefines.size(),
        (IDxcCompilerArgs**)&compilerArgs
    ));

    DxcBuffer sourceBuffer = {
        codeBlob->GetBufferPointer(),
        codeBlob->GetBufferSize(),
        0u };

    SmartPtr<IDxcResult> results;

    DX_OK(compiler.Compile(
        &sourceBuffer,
        compilerArgs->GetArguments(),
        compilerArgs->GetCount(),
        nullptr, /*todo place header*/
        __uuidof(IDxcResult),
        (void**)&results
    ));

    int numOutputs = (int)results->GetNumOutputs();
    bool compiledSuccess = false;
    for (int i = 0; i < numOutputs; ++i)
    {
        DXC_OUT_KIND kind = results->GetOutputByIndex(i);
        if (kind == DXC_OUT_ERRORS)
        {
            SmartPtr<IDxcBlobUtf8> errorBlob;
            DX_OK(results->GetOutput(
                DXC_OUT_ERRORS,
                __uuidof(IDxcBlobUtf8),
                (void**)&errorBlob,
                nullptr));
            
            if (args.onError)
            {
                std::string errorStr;
                if (errorBlob != nullptr && errorBlob->GetStringLength() > 0)
                {
                    errorStr.assign(errorBlob->GetStringPointer(), errorBlob->GetStringLength());
                    args.onError(args.shaderName, errorStr.c_str());
                }
            }
        }
        else if (kind == DXC_OUT_OBJECT)
        {
            SmartPtr<IDxcBlob> shaderOut;
            DX_OK(results->GetOutput(
                DXC_OUT_OBJECT,
                __uuidof(IDxcBlob),
                (void**)&shaderOut,
                nullptr));

            if (shaderOut != nullptr)
            {
                if (args.onFinished)
                    args.onFinished(true, &(*shaderOut));
                compiledSuccess = true;
            }
            else if (args.onError)
            {
                args.onError(args.shaderName, "Failed to retrieve result blob.");
            }
        }
    }

    if (!compiledSuccess && args.onFinished)
    {
        args.onFinished(false, nullptr);
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
}


}

#endif
