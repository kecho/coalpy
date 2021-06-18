#include "testsystem.h"
#include <coalpy.core/Assert.h>
#include <coalpy.tasks/ITaskSystem.h>
#include <coalpy.files/IFileSystem.h>
#include <coalpy.files/Utils.h>
#include <coalpy.render/IShaderDb.h>
#include <coalpy.render/../../Config.h>
#include <coalpy.render/../../Dx12/Dx12Compiler.h>
#include <sstream>
#include <iostream>
#include <atomic>

namespace coalpy
{

class ShaderServiceContext : public TestContext
{
public:
    ITaskSystem* ts = nullptr;
    IFileSystem* fs = nullptr;
    IShaderDb* db = nullptr;
    std::string rootDir;
    virtual ~ShaderServiceContext() {}
    void begin()
    {
        ts->start();
    }

    void end()
    {
        ts->signalStop();
        ts->join();
        ts->cleanFinishedTasks();
    }
};

const char* simpleComputeShader()
{
    return R"(
        Texture2D<float> input : register(t0);
        RWTexture2D<float> output : register(u0);
        [numthreads(8,8,1)]
        void csMain(int3 dti : SV_DispatchThreadID)
        {
            output[dti.xy] = input[dti.xy];
        }
    )";
}


const char* simpleComputeShaderWithInclude()
{
    return R"(
        #include "testInclude.hlsl"
        Texture2D<float> input : register(t0);
        RWTexture2D<float> output : register(u0);
        [numthreads(8,8,1)]
        void csMain(int3 dti : SV_DispatchThreadID)
        {
            output[dti.xy] = input[dti.xy] + g_someFloat4.w;
        }
    )";
}

const char* simpleComputeInclude()
{
    return R"(
        cbuffer buff : register(b0)
        {
            float4 g_someFloat4;
        }
    )";
}

#if ENABLE_DX12
void dx12CompileFunction(Dx12Compiler& compiler)
{
    Dx12CompileArgs compilerArgs = {};
    compilerArgs.type = ShaderType::Compute;
    compilerArgs.mainFn = "csMain";
    compilerArgs.shaderName = "SimpleComputeShader";
    compilerArgs.source = simpleComputeShader();
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

    int compileTasksCount = 20;
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

    int compileTasksCount = 20;
    for (int i = 0; i < compileTasksCount; ++i)
    {
        dx12CompileFunction(compiler);
    }
}
#endif

void shaderDbCompile(TestContext& ctx)
{
    auto& testContext = (ShaderServiceContext&)ctx;
    testContext.begin();
    IFileSystem& fs = *testContext.fs;
    ITaskSystem& ts = *testContext.ts;
    IShaderDb& db = *testContext.db;

    fs.carveDirectoryPath("shaderTest");

    //write simple include
    std::string includeName = "shaderTest/testInclude.hlsl";
    AsyncFileHandle includeFile = fs.write(FileWriteRequest(includeName, [](FileWriteResponse& response) {}, simpleComputeInclude(), strlen(simpleComputeInclude())));

    //write tmp shader files
    std::vector<std::string> fileNames;
    std::vector<AsyncFileHandle> files;
    std::atomic<int> successCount = 0;
    Task allWrite = ts.createTask();
    ts.depends(allWrite, fs.asTask(includeFile));
    for (int i = 0; i < 400; ++i)
    {
        std::stringstream name;
        name << "shaderTest/testShader-" << i <<  ".hlsl";
        fileNames.push_back(name.str());
        files.push_back(fs.write(FileWriteRequest(fileNames.back(),
            [&successCount](FileWriteResponse& response)
            {
                CPY_ASSERT_FMT(response.status != FileStatus::Fail, "failed writting file %s", IoError2String(response.error));
                if (response.status == FileStatus::Success)
                    ++successCount;
            },
            simpleComputeShaderWithInclude(), strlen(simpleComputeShaderWithInclude())
        )));

        ts.depends(allWrite, fs.asTask(files.back()));
    }

    ts.execute(allWrite);
    ts.wait(allWrite);
    int finalCount = successCount;
    CPY_ASSERT_FMT(finalCount == (int)fileNames.size(), "Failed to write %d files, wrote only %d", (int)fileNames.size(), finalCount) ;

    for (auto fileHandle : files)
        fs.closeHandle(fileHandle);
    fs.closeHandle(includeFile);
    files.clear();
    ts.cleanTaskTree(allWrite);

    std::vector<ShaderHandle> shaderHandles;
    for (const auto& fileName : fileNames)
    {
        ShaderDesc sd;
        sd.type = ShaderType::Compute;
        sd.name = fileName.c_str();
        sd.mainFn = "csMain";
        sd.path = fileName.c_str();
        shaderHandles.push_back(db.requestCompile(sd));
    }

    for (auto h : shaderHandles)
    {
        db.resolve(h);
        CPY_ASSERT(db.isValid(h));
    }

    {
        std::vector<std::string> dirList;
        fs.enumerateFiles("shaderTest", dirList);
        for (const auto& d: dirList)
        {
            FileAttributes attributes = {};
            fs.getFileAttributes(d.c_str(), attributes);
            if (attributes.exists && !attributes.isDot && !attributes.isDir)
            {
                bool deletedFile = fs.deleteFile(d.c_str()); 
                CPY_ASSERT_FMT(deletedFile, "Could not delete file %s", d.c_str());
            }
        }
        bool clearTestDir = fs.deleteDirectory("shaderTest");
        CPY_ASSERT_MSG(clearTestDir, "Could not clear test directory 'shaderTest', ensure all files have been deleted");
    }
    testContext.end();
}

void testFileWatch(TestContext& ctx)
{
    auto& testContext = (ShaderServiceContext&)ctx;
    testContext.begin();
    testContext.end();
}

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
            { "shaderDbCompile", shaderDbCompile },
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
        std::string resourceDir = rootDir + (rootDir == "" ? ".\\" : "\\") + "coalpy\\resources\\"; 

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
            ShaderDbDesc desc = { resourceDir.c_str(), testContext->fs, testContext->ts };
            testContext->db = IShaderDb::create(desc);
        }

        return testContext;
    }

    virtual void destroyContext(TestContext* context)
    {
        auto testContext = static_cast<ShaderServiceContext*>(context);
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
