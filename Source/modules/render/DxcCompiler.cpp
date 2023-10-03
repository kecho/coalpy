#include <Config.h>
#include "DxcCompiler.h"
#include <coalpy.core/Assert.h>
#include <coalpy.core/GenericHandle.h>
#include <coalpy.core/HandleContainer.h>
#include <coalpy.core/SmartPtr.h>
#include <coalpy.core/String.h>
#include <coalpy.core/ClTokenizer.h>
#include <coalpy.core/ByteBuffer.h>
#include <coalpy.core/RefCounted.h>
#include <coalpy.files/Utils.h>
#include <iostream>
#include <mutex>

#include <sstream>
#include <string>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#elif defined(__linux__)
#include <dlfcn.h>
#elif defined(__APPLE__)
#include <dlfcn.h>
#include <spirv_msl.hpp>
#endif

#include <dxcapi.h>
#include "SpirvReflectionData.h"

namespace coalpy
{

namespace 
{

DxcCreateInstanceProc g_dxcCreateInstanceFn = nullptr;
DxcCreateInstanceProc g_dxilCreateInstanceFn = nullptr;

#ifdef _WIN32
typedef HMODULE LIB_MODULE ;
#else
typedef void* LIB_MODULE;
#endif

LIB_MODULE g_dxcModule = nullptr;
#ifdef _WIN32
LIB_MODULE g_dxilModule = nullptr;
const char* g_defaultDxcPath = "coalpy\\resources";
const char* g_dxCompiler = "dxcompiler.dll";
const char* g_dxil = "dxil.dll";
#elif defined(__linux__)
const char* g_defaultDxcPath = "coalpy/resources";
const char* g_dxCompiler = "libdxcompiler.so";
#elif defined(__APPLE__)
const char* g_defaultDxcPath = "coalpy/resources";
const char* g_dxCompiler = "libdxcompiler.dylib";
#else
#error "Unknown platform"
#endif

void loadCompilerModule(const char* searchPath, const char* moduleName, LIB_MODULE& outModule, DxcCreateInstanceProc& outProc)
{
    std::string compilerPath = searchPath ? searchPath : g_defaultDxcPath;
    std::stringstream compilerFullPath;
    if (compilerPath != "")
#ifdef _WIN32
        compilerFullPath << compilerPath << "\\";
#elif defined(__linux__) || defined(__APPLE__)
        compilerFullPath << compilerPath << "/";
#else
#error "Unknown platform"
#endif
    compilerFullPath << moduleName;
    std::string pathAndModName = compilerFullPath.str();
    
    std::string fullModulePath;
    FileUtils::getAbsolutePath(pathAndModName, fullModulePath);
#ifdef _WIN32
    outModule = LoadLibraryA(fullModulePath.c_str());
#elif defined(__linux__) || defined(__APPLE__)
    outModule = dlopen(fullModulePath.c_str(), RTLD_GLOBAL | RTLD_NOW);
#else
#error "Unknown platform"
#endif
    if (outModule == nullptr)
    {
        std::cerr << "Failed to load DXC module \"" << fullModulePath.c_str() << "\"" << std::endl;
    }
    CPY_ASSERT_FMT(outModule != nullptr, "Error: could not load %s. Missing dll.", fullModulePath.c_str());
    
    if (outModule)
    {
#ifdef _WIN32
        outProc = (DxcCreateInstanceProc)GetProcAddress(outModule, "DxcCreateInstance");
#elif defined(__linux__) || defined(__APPLE__)
        outProc = (DxcCreateInstanceProc)dlsym(outModule, "DxcCreateInstance");
#endif
        CPY_ASSERT_FMT(outProc, "Could not find \"DxcCreateInstance\" inside %s", pathAndModName.c_str());
    }
}

struct DxcCompilerHandle : public GenericHandle<unsigned int> {};
struct DxcValidatorHandle : public GenericHandle<unsigned int> {};
struct DxcUtilsHandle : public GenericHandle<unsigned int>{};

struct DxcInstance
{
    DxcCompilerHandle compiler;
    DxcUtilsHandle utils;
    DxcValidatorHandle validator;
};

struct DxcInstanceData
{
    IDxcCompiler3& compiler;
    IDxcUtils& utils;
    IDxcValidator2* validator;
};

class DxcPool : public RefCounted
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
            SmartPtr<IDxcUtils>& instance = m_utilsPool.allocate(h.utils);
            if (instance == nullptr)
                DX_OK(g_dxcCreateInstanceFn(CLSID_DxcUtils, __uuidof(IDxcUtils), (void**)&instance));
        }

        {
#ifdef _WIN32
            SmartPtr<IDxcValidator2>& instance = m_validatorPool.allocate(h.validator);
            if (instance == nullptr)
                DX_OK(g_dxilCreateInstanceFn(CLSID_DxcValidator, __uuidof(IDxcValidator2), (void**)&instance));
#endif
        }
        return h;
    }

    DxcInstanceData operator [](DxcInstance instance)
    {
        CPY_ERROR(m_compilerPool.contains(instance.compiler));
        CPY_ERROR(m_utilsPool.contains(instance.utils));
        SmartPtr<IDxcCompiler3>& c = m_compilerPool[instance.compiler];
        SmartPtr<IDxcValidator2> vc;
        if (instance.validator.valid())
            vc = m_validatorPool[instance.validator];
        SmartPtr<IDxcUtils>& u = m_utilsPool[instance.utils];
        return DxcInstanceData {
            *c,
            *u,
            vc,
        };
    }

    void release(DxcInstance instance)
    {
        m_compilerPool.free(instance.compiler, false/*dont reset object so we recycle it*/);
        m_utilsPool.free(instance.utils, false/*dont reset object so we recycle it*/);
        m_validatorPool.free(instance.validator, false/*dont reset object so we recycle it*/);
    }

    void clear()
    {
        m_compilerPool.clear();
        m_utilsPool.clear();
        m_validatorPool.clear();
    }

private:
    HandleContainer<DxcCompilerHandle, SmartPtr<IDxcCompiler3>> m_compilerPool;
    HandleContainer<DxcUtilsHandle, SmartPtr<IDxcUtils>> m_utilsPool;
    HandleContainer<DxcValidatorHandle, SmartPtr<IDxcValidator2>> m_validatorPool;
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
    DxcIncludeHandler(IDxcUtils& utils, DxcCompilerOnInclude includeFn)
    : m_includeFn(includeFn)
    , m_utils(utils)
    {
    }

    virtual HRESULT LoadSource(LPCWSTR pFilename, IDxcBlob **ppIncludeSource) override
    {
        m_buffer.resize(0u);
        std::wstring fileName = pFilename;
        std::string sfileName = ws2s(fileName);
        std::string resolvedPath;
        FileUtils::getAbsolutePath(sfileName, resolvedPath);
        IDxcBlobEncoding* codeBlob = nullptr;
        if (m_includeFn(resolvedPath.c_str(), m_buffer))
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

    DxcCompilerOnInclude m_includeFn;
    IDxcUtils& m_utils;
    ByteBuffer m_buffer;
};

}

DxcCompiler::DxcCompiler(const ShaderDbDesc& desc)
: m_desc(desc)
{
    setupDxc();
}

DxcCompiler::~DxcCompiler()
{
}

const wchar_t** getShaderModelTargets(ShaderModel sm)
{
    static const wchar_t* sm6_0_targets[(int)ShaderType::Count] = {
        L"vs_6_0", //Vertex
        L"ps_6_0", //Pixel
        L"cs_6_0"  //Compute
    };
    static const wchar_t* sm6_1_targets[(int)ShaderType::Count] = {
        L"vs_6_1", //Vertex
        L"ps_6_1", //Pixel
        L"cs_6_1"  //Compute
    };
    static const wchar_t* sm6_2_targets[(int)ShaderType::Count] = {
        L"vs_6_2", //Vertex
        L"ps_6_2", //Pixel
        L"cs_6_2"  //Compute
    };
    static const wchar_t* sm6_3_targets[(int)ShaderType::Count] = {
        L"vs_6_3", //Vertex
        L"ps_6_3", //Pixel
        L"cs_6_3"  //Compute
    };
    static const wchar_t* sm6_4_targets[(int)ShaderType::Count] = {
        L"vs_6_4", //Vertex
        L"ps_6_4", //Pixel
        L"cs_6_4"  //Compute
    };
    static const wchar_t* sm6_5_targets[(int)ShaderType::Count] = {
        L"vs_6_5", //Vertex
        L"ps_6_5", //Pixel
        L"cs_6_5"  //Compute
    };

    switch (sm)
    {
    case ShaderModel::Sm6_0:
        return sm6_0_targets;
    case ShaderModel::Sm6_1:
        return sm6_1_targets;
    case ShaderModel::Sm6_2:
        return sm6_2_targets;
    case ShaderModel::Sm6_3:
        return sm6_3_targets;
    case ShaderModel::Sm6_4:
        return sm6_4_targets;
    case ShaderModel::Sm6_5:
    default:
        return sm6_5_targets;
    }
}

thread_local bool g_registerShiftCached = false;
thread_local std::vector<std::wstring> g_registerShiftArgs;
void addSpirvArguments(std::vector<std::wstring>& args)
{
    if (!g_registerShiftCached)
    {
        for (int s = 0; s < (int)SpirvMaxRegisterSpace; ++s)
        {
            for (int t = 0; t < (int)SpirvRegisterType::Count; ++t)
            {
                auto regType = (SpirvRegisterType)t;
                {
                    std::wstringstream ss;
                    ss << L"-fvk-" << SpirvRegisterTypeName(regType) << L"-shift";
                    g_registerShiftArgs.push_back(ss.str());
                }

                {
                    std::wstringstream ss;
                    ss << SpirvRegisterTypeOffset(regType);
                    g_registerShiftArgs.push_back(ss.str());
                }

                {
                    std::wstringstream ss;
                    ss << s;
                    g_registerShiftArgs.push_back(ss.str());
                }
            }
        }
        g_registerShiftCached = true;
    }

    args.push_back(L"-spirv");
    //TODO: uncoment to get support for validated spirv append consume buffers
    //args.push_back(L"-fspv-reflect"); //add the append consume buffer reflection info
    for (auto& s : g_registerShiftArgs)
    {
        args.push_back(s.c_str());
    }
}

void DxcCompiler::compileShader(const DxcCompileArgs& args)
{
    CPY_ASSERT(args.shaderModel >= ShaderModel::Begin && args.shaderModel <= ShaderModel::End);
    const wchar_t** smTargets = getShaderModelTargets(args.shaderModel);

    DxcCompilerScope scope;
    DxcInstanceData instanceData = scope.data();
    IDxcCompiler3& compiler = instanceData.compiler;
    IDxcUtils& utils = instanceData.utils;

    SmartPtr<IDxcBlobEncoding> codeBlob;
    DX_OK(utils.CreateBlob(args.source, args.sourceSize > 0 ? args.sourceSize : strlen(args.source), CP_UTF8, (IDxcBlobEncoding**)&codeBlob));

    std::string  sshaderName = args.debugName ? args.debugName : args.shaderName;
    std::wstring wshaderName = s2ws(sshaderName);

    std::string  sentryPoint = args.mainFn;
    std::wstring wentryPoint = s2ws(sentryPoint);

    const wchar_t* profile = smTargets[(int)args.type];

    const bool outputSpirV =
        (m_desc.platform == render::DevicePlat::Vulkan) ||
        (m_desc.platform == render::DevicePlat::Metal);
    const bool generatePdb = !outputSpirV  && args.generatePdb;

    std::vector<LPCWSTR> arguments;
    arguments.push_back(DXC_ARG_WARNINGS_ARE_ERRORS);
    arguments.push_back(DXC_ARG_OPTIMIZATION_LEVEL3);
    arguments.push_back(DXC_ARG_ENABLE_STRICTNESS);
    if (generatePdb)
        arguments.push_back(DXC_ARG_DEBUG);

    std::vector<std::wstring> paths;
    if (outputSpirV)
        addSpirvArguments(paths);

    paths.push_back(L"-I.");
    paths.push_back(L"-Icoalpy");

    for (auto& incPath : args.additionalIncludes)
    {
        std::stringstream ss;
        ss << "-I" << incPath;
        std::wstring pathArg = s2ws(ss.str());
        paths.push_back(pathArg);
    }

    for (auto& path : paths)
    {
        arguments.push_back(path.c_str());
    }

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

            SmartPtr<IDxcBlob> pdbOut;
            SmartPtr<IDxcBlobWide> pdbName;
            if (generatePdb)
            {
                DX_OK(results->GetOutput(
                    DXC_OUT_PDB,
                    __uuidof(IDxcBlob),
                    (void**)&pdbOut,
                    (IDxcBlobWide**)(&pdbName)));
            }

            if (shaderOut != nullptr)
            {
                if (!outputSpirV && instanceData.validator)
                {
                    SmartPtr<IDxcOperationResult> validationResult;
                    DX_OK(instanceData.validator->Validate(&(*shaderOut), DxcValidatorFlags_InPlaceEdit, (IDxcOperationResult**)&validationResult));

                    HRESULT validationStatus;
                    validationResult->GetStatus(&validationStatus);
                    if (FAILED(validationStatus))
                    {
                        if (args.onError)
                            compiledSuccess = false;
                    }
                }

                compiledSuccess = compiledSuccess && shaderOut->GetBufferPointer() != nullptr;
                if (args.onFinished)
                {
                    SpirvReflectionData* spirVReflectionData = nullptr;
                    if (outputSpirV)
                    {
                        spirVReflectionData = new SpirvReflectionData();
                        if (!spirVReflectionData->load(shaderOut->GetBufferPointer(), shaderOut->GetBufferSize()))
                        {
                            spirVReflectionData->Release();
                            spirVReflectionData = nullptr;
                        }
                        else
                            spirVReflectionData->mainFn = args.mainFn;
                    }
                    // ---
                    {
                        std::cout << "----------\n";
                        std::cout <<shaderOut->GetBufferSize()<<"\n";

                        spirv_cross::CompilerGLSL glsl((uint*)shaderOut->GetBufferPointer(), shaderOut->GetBufferSize() / 4);
                        spirv_cross::ShaderResources resources = glsl.get_shader_resources();
                        for (auto &resource : resources.separate_images)
                        {
                            printf("%s\n", resource.name.c_str());
                        }
                        for (auto &resource : resources.builtin_inputs)
                        {
                            printf("%d\n", resource.value_type_id);
                        }
                        std::cout << "----------\n";

                    }
                    // ---

                    DxcResultPayload payload = {};
                    payload.resultBlob = &(*shaderOut);
                    payload.pdbBlob = pdbOut == nullptr ? nullptr : &(*pdbOut);
                    payload.pdbName = pdbName == nullptr ? nullptr : &(*pdbName);
                    payload.spirvReflectionData = spirVReflectionData;
                    args.onFinished(true, payload);

                    if (spirVReflectionData)
                        spirVReflectionData->Release();
                }
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
            DxcResultPayload payload = {};
            args.onFinished(false, payload);
        }

    }
}

void DxcCompiler::setupDxc()
{
    if (g_dxcModule == nullptr)
        loadCompilerModule(m_desc.compilerDllPath.c_str(), g_dxCompiler, g_dxcModule, g_dxcCreateInstanceFn);

#ifdef _WIN32
    if (g_dxilModule == nullptr)
        loadCompilerModule(m_desc.compilerDllPath.c_str(), g_dxil, g_dxilModule, g_dxilCreateInstanceFn);
#endif
}

}
