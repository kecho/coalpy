#include "testsystem.h"
#include <coalpy.core/Assert.h>
#include <coalpy.tasks/ITaskSystem.h>
#include <coalpy.files/IFileSystem.h>
#include <coalpy.files/Utils.h>
#include <coalpy.shader/IShaderService.h>
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

class ShaderSuite : public TestSuite
{
public:
    virtual const char* name() const { return "shader"; }
    virtual const TestCase* getCases(int& caseCounts) const
    {
        static TestCase sCases[] = {
            { "testFilewatch", testFileWatch }
        };

        caseCounts = (int)(sizeof(sCases) / sizeof(TestCase));
        return sCases;
    }

    virtual TestContext* createContext()
    {
        auto testContext = new ShaderServiceContext();

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
            std::string rootDir;
            auto appContext = ApplicationContext::get();
            std::string appName = appContext.argv[0];
            FileUtils::getDirName(appName, rootDir);
            std::cout << rootDir << std::endl;
            ShaderServiceDesc desc { testContext->fs, testContext->ts, rootDir.c_str(), 60, nullptr, nullptr };
            testContext->ss = IShaderService::create(desc);
        }

        return testContext;
    }

    virtual void destroyContext(TestContext* context)
    {
        auto testContext = static_cast<ShaderServiceContext*>(context);
        delete testContext->ss;
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
