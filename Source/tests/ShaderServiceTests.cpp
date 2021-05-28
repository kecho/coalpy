#include "testsystem.h"
#include <coalpy.core/Assert.h>
#include <coalpy.tasks/ITaskSystem.h>
#include <coalpy.files/IFileSystem.h>
#include <coalpy.files/Utils.h>
#include <coalpy.shader/IShaderService.h>
#include <coalpy.render/IShaderDb.h>
#include <coalpy.render/../../Config.h>
#include <coalpy.render/../../Dx12/Dx12Compiler.h>
#include <sstream>
#include <iostream>

namespace coalpy
{

class ShaderServiceContext : public TestContext
{
public:
    ITaskSystem* ts = nullptr;
    IFileSystem* fs = nullptr;
    IShaderService* ss = nullptr;
    IShaderDb* db = nullptr;
    std::string rootDir;

    void begin()
    {
        ts->start();
        ss->start();
    }

    void end()
    {
        ss->stop();
        ts->signalStop();
        ts->join();
        ts->cleanFinishedTasks();
    }
};

void testFileWatch(TestContext& ctx)
{
    auto& testContext = (ShaderServiceContext&)ctx;
    testContext.begin();
    testContext.end();
}

#if ENABLE_DX12
void dx12CompileFunction(Dx12Compiler& compiler)
{
    Dx12CompileArgs compilerArgs = {};
    compilerArgs.type = ShaderType::Compute;
    compilerArgs.mainFn = "csMain";
    compilerArgs.shaderName = "SimpleComputeShader";
    compilerArgs.source = R"(
        Texture2D<float> input : register(t0);
        RWTexture2D<float> output : register(u0);
        [numthreads(8,8,1)]
        void csMain(int3 dti : SV_DispatchThreadID)
        {
            output[dti.xy] = input[dti.xy];
        }
    )";

    compilerArgs.onError = [&](const char* name, const char* errorString)
    {
        CPY_ASSERT_FMT(false, "Unexpected compilation error: \"%s\".", errorString);
    };

    int onFinishedReached = 0;
    compilerArgs.onFinished = [&](bool success, IDxcBlob* blob)
    {
        ++onFinishedReached;
        bool compiledSuccess = success && blob != nullptr;
        CPY_ASSERT_MSG(compiledSuccess, "Unexpected compilation error.");
    };

    compiler.compileShader(compilerArgs);
    CPY_ASSERT_FMT(onFinishedReached == 1, "onFinishedFunction reached multiple times %d.", onFinishedReached);
}

void dx12TestSimpleDxcCompile(TestContext& ctx)
{
    auto& testContext = (ShaderServiceContext&)ctx;

    ShaderDbDesc dbDesc;
    dbDesc.compilerDllPath = testContext.rootDir.c_str();
    Dx12Compiler compiler(dbDesc);
    dx12CompileFunction(compiler);
}

void dx12TestParallelDxcCompile(TestContext& ctx)
{
    auto& testContext = (ShaderServiceContext&)ctx;
    testContext.begin();

    ShaderDbDesc dbDesc;
    dbDesc.compilerDllPath = testContext.rootDir.c_str();
    Dx12Compiler compiler(dbDesc);

    TaskDesc compileTask([&compiler](TaskContext& taskContext) {
        dx12CompileFunction(compiler);
    });

    int compileTasksCount = 200;
    std::vector<Task> compileTasks;
    for (int i = 0; i < compileTasksCount; ++i)
        compileTasks.push_back(testContext.ts->createTask(compileTask));

    Task rootTask = testContext.ts->createTask();
    testContext.ts->depends(rootTask, compileTasks.data(), (int)compileTasks.size());

    testContext.ts->execute(rootTask);
    testContext.ts->wait(rootTask);
    testContext.ts->cleanTaskTree(rootTask);

    testContext.end();
}

void dx12TestManySerialDxcCompile(TestContext& ctx)
{
    auto& testContext = (ShaderServiceContext&)ctx;
    ShaderDbDesc dbDesc;
    dbDesc.compilerDllPath = testContext.rootDir.c_str();
    Dx12Compiler compiler(dbDesc);

    int compileTasksCount = 200;
    for (int i = 0; i < compileTasksCount; ++i)
    {
        dx12CompileFunction(compiler);
    }
}
#endif

class ShaderSuite : public TestSuite
{
public:
    virtual const char* name() const { return "shader"; }
    virtual const TestCase* getCases(int& caseCounts) const
    {
        static TestCase sCases[] = {
#if ENABLE_DX12
            { "dx12TestSimpleDxcCompile", dx12TestSimpleDxcCompile },
            { "dx12TestManyParallelDxcCompile", dx12TestParallelDxcCompile },
            { "dx12TestManySerialDxcCompile", dx12TestManySerialDxcCompile },
#endif
            { "testFilewatch", testFileWatch }
        };

        caseCounts = (int)(sizeof(sCases) / sizeof(TestCase));
        return sCases;
    }

    virtual TestContext* createContext()
    {
        auto testContext = new ShaderServiceContext();

        std::string rootDir;
        auto appContext = ApplicationContext::get();
        std::string appName = appContext.argv[0];
        FileUtils::getDirName(appName, rootDir);

        testContext->rootDir = rootDir;

        {
            TaskSystemDesc desc;
            desc.threadPoolSize = 8;
            testContext->ts = ITaskSystem::create(desc);
        }

        {
            FileSystemDesc desc { testContext->ts };
            testContext->fs = IFileSystem::create(desc);
        }

        {
            ShaderDbDesc desc;
            desc.compilerDllPath = rootDir.c_str();
            testContext->db = IShaderDb::create(desc);
        }

        {
            ShaderServiceDesc desc { testContext->fs, testContext->ts, testContext->db, rootDir.c_str(), 60, nullptr, nullptr };
            testContext->ss = IShaderService::create(desc);
        }

        return testContext;
    }

    virtual void destroyContext(TestContext* context)
    {
        auto testContext = static_cast<ShaderServiceContext*>(context);
        delete testContext->ss;
        delete testContext->db;
        delete testContext->fs;
        delete testContext->ts;
        delete testContext;
    }
};

TestSuite* shaderSuite()
{
    return new ShaderSuite;
}

}
