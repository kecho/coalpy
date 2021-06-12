#include <Config.h>
#include "Dx12Compiler.h"
#if ENABLE_DX12

#include <coalpy.core/Assert.h>
#include <coalpy.core/GenericHandle.h>
#include <coalpy.core/HandleContainer.h>
#include <coalpy.core/SmartPtr.h>
#include <coalpy.core/String.h>
#include <coalpy.core/ClTokenizer.h>
#include <coalpy.core/ByteBuffer.h>
#include <coalpy.files/Utils.h>
#include <mutex>

#include <string>
#include <vector>
#include <windows.h>
#include <dxc/inc/dxcapi.h>

namespace coalpy
{

namespace 
{

const char* g_defaultDxcPath = "coalpy\\resources";
const char* g_dxCompiler = "dxcompiler.dll";
const char* g_dxil = "dxil.dll";
HMODULE g_dxcModule = nullptr;
HMODULE g_dxilModule = nullptr;

DxcCreateInstanceProc g_dxcCreateInstanceFn = nullptr;
DxcCreateInstanceProc g_dxilCreateInstanceFn = nullptr;

void loadCompilerModule(const char* searchPath, const char* moduleName, HMODULE& outModule, DxcCreateInstanceProc& outProc)
{
    std::string compilerPath = searchPath ? searchPath : g_defaultDxcPath;
    std::stringstream compilerFullPath;
    if (compilerPath != "")
        compilerFullPath << compilerPath << "\\";
    compilerFullPath << moduleName;
    std::string pathAndModName = compilerFullPath.str();
    
    std::string fullModulePath;
    FileUtils::getAbsolutePath(pathAndModName, fullModulePath);
    outModule = LoadLibraryA(fullModulePath.c_str());
    CPY_ASSERT_FMT(outModule != nullptr, "Error: could not load %s. Missing dll.", pathAndModName.c_str());
    
    if (outModule)
    {
        outProc = (DxcCreateInstanceProc)GetProcAddress(outModule, "DxcCreateInstance");
        CPY_ASSERT_FMT(outProc, "Could not find \"DxcCreateInstance\" inside %s", pathAndModName.c_str());
    }
}

struct DxcCompilerHandle : public GenericHandle<unsigned int> {};
struct DxcValidatorHandle : public GenericHandle<unsigned int> {};
struct DxcUtilsHandle : public GenericHandle<unsigned int>{};

struct DxcInstance
{
    DxcCompilerHandle compiler;
    DxcValidatorHandle validator;
    DxcUtilsHandle utils;
};

struct DxcInstanceData
{
    IDxcCompiler3& compiler;
    IDxcValidator2& validator;
    IDxcUtils& utils;
};

class DxcPool
{
public:
    DxcPool()
    {
    }

    DxcInstance allocate()
    {
        DxcInstance h;

        {
            SmartPtr<IDxcCompiler3>& instance = m_compilerPool.allocate(h.compiler);
            if (instance == nullptr)
                DX_OK(g_dxcCreateInstanceFn(CLSID_DxcCompiler, __uuidof(IDxcCompiler3), (void**)&instance));
        }

        {
            SmartPtr<IDxcValidator2>& instance = m_validatorPool.allocate(h.validator);
            if (instance == nullptr)
                DX_OK(g_dxilCreateInstanceFn(CLSID_DxcValidator, __uuidof(IDxcValidator2), (void**)&instance));
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
        SmartPtr<IDxcValidator2>& vc = m_validatorPool[instance.validator];
        SmartPtr<IDxcUtils>& u = m_utilsPool[instance.utils];
        return DxcInstanceData { *c, *vc, *u };
    }

    void release(DxcInstance instance)
    {
        m_compilerPool.free(instance.compiler, false/*dont reset object so we recycle it*/);
        m_validatorPool.free(instance.validator, false/*dont reset object so we recycle it*/);
        m_utilsPool.free(instance.utils, false/*dont reset object so we recycle it*/);
    }

    void clear()
    {
        m_compilerPool.clear();
        m_validatorPool.clear();
        m_utilsPool.clear();
    }

    void AddRef()
    {
        ++m_ref;
    }

    void Release()
    {
        --m_ref;
        if (m_ref <= 0)
            delete this;
    }

private:
    HandleContainer<DxcCompilerHandle, SmartPtr<IDxcCompiler3>> m_compilerPool;
    HandleContainer<DxcUtilsHandle, SmartPtr<IDxcUtils>> m_utilsPool;
    HandleContainer<DxcValidatorHandle, SmartPtr<IDxcValidator2>> m_validatorPool;
    int m_ref = 0;
};

thread_local SmartPtr<DxcPool> s_dxcPool = nullptr;

struct DxcCompilerScope
{
    DxcCompilerScope()
    {
        if (s_dxcPool == nullptr)
            s_dxcPool = new DxcPool;
        m_instance = s_dxcPool->allocate();
    }

    ~DxcCompilerScope()
    {
        s_dxcPool->release(m_instance);
    }

    inline DxcInstanceData data() { return (*s_dxcPool)[m_instance]; }
    DxcInstance m_instance;
};

struct DxcIncludeHandler : public IDxcIncludeHandler
{
    DxcIncludeHandler(IDxcUtils& utils, Dx12CompilerOnInclude includeFn)
    : m_includeFn(includeFn)
    , m_utils(utils)
    {
    }

    virtual HRESULT LoadSource(LPCWSTR pFilename, IDxcBlob **ppIncludeSource) override
    {
        m_buffer.resize(0u);
        std::wstring fileName = pFilename;
        std::string sfileName = ws2s(fileName);
        IDxcBlobEncoding* codeBlob = nullptr;
        if (m_includeFn(sfileName.c_str(), m_buffer))
        {
            DX_OK(m_utils.CreateBlob(m_buffer.data(), m_buffer.size(), CP_UTF8, &codeBlob));
            CPY_ASSERT(codeBlob != nullptr);
        }

        *ppIncludeSource = codeBlob;
        return S_OK;
    }

    virtual ULONG AddRef() override
    {
        return 1u;
    }

    virtual ULONG Release() override
    {
        return 1u;
    }

    virtual HRESULT QueryInterface(const IID &,void ** ptr)
    {
        *ptr = this;
        return S_OK;
    }

    Dx12CompilerOnInclude m_includeFn;
    IDxcUtils& m_utils;
    ByteBuffer m_buffer;
};

}

Dx12Compiler::Dx12Compiler(const ShaderDbDesc& desc)
: m_desc(desc)
{
    setupDxc();
}

Dx12Compiler::~Dx12Compiler()
{
    //DxcPool::collectGarbage();
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
    DX_OK(utils.CreateBlob(args.source, args.sourceSize > 0 ? args.sourceSize : strlen(args.source), CP_UTF8, (IDxcBlobEncoding**)&codeBlob));

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

    DxcIncludeHandler includeHandler(utils, args.onInclude);
    DX_OK(compiler.Compile(
        &sourceBuffer,
        compilerArgs->GetArguments(),
        compilerArgs->GetCount(),
        args.onInclude == nullptr ? nullptr : &includeHandler,
        __uuidof(IDxcResult),
        (void**)&results
    ));

    bool compiledSuccess = true;
    {
        //DXC_OUT_OBJECT
        {
            SmartPtr<IDxcBlob> shaderOut;
            DX_OK(results->GetOutput(
                DXC_OUT_OBJECT,
                __uuidof(IDxcBlob),
                (void**)&shaderOut,
                nullptr));

            if (shaderOut != nullptr)
            {
                SmartPtr<IDxcOperationResult> validationResult;
                DX_OK(instanceData.validator.Validate(&(*shaderOut), DxcValidatorFlags_InPlaceEdit, (IDxcOperationResult **)&validationResult));

                HRESULT validationStatus;
                validationResult->GetStatus(&validationStatus);
                if (FAILED(validationStatus))
                {
                    if (args.onError)
                        compiledSuccess = false;
                }
                else if (args.onFinished)
                    args.onFinished(true, &(*shaderOut));
            }
            else if (args.onError)
            {
                compiledSuccess = false;
                args.onError(args.shaderName, "Failed to retrieve result blob.");
            }
        }

        //DXC_OUT_ERRORS
        if (!compiledSuccess)
        {
            SmartPtr<IDxcBlobUtf8> errorBlob;
            DX_OK(results->GetOutput(
                DXC_OUT_ERRORS,
                __uuidof(IDxcBlobUtf8),
                (void**)&errorBlob,
                nullptr));
            

            std::string errorStr;
            if (errorBlob != nullptr && errorBlob->GetStringLength() > 0)
            {
                if (args.onError)
                {
                    errorStr.assign(errorBlob->GetStringPointer(), errorBlob->GetStringLength());
                    args.onError(args.shaderName, errorStr.c_str());
                }
                compiledSuccess = false;
            }
            args.onFinished(false, nullptr);
        }

    }
}

void Dx12Compiler::setupDxc()
{
    if (g_dxcModule == nullptr)
        loadCompilerModule(m_desc.compilerDllPath, g_dxCompiler, g_dxcModule, g_dxcCreateInstanceFn);

    if (g_dxilModule == nullptr)
        loadCompilerModule(m_desc.compilerDllPath, g_dxil, g_dxilModule, g_dxilCreateInstanceFn);
}


}

#endif