#include "testsystem.h"
#include <coalpy.core/Assert.h>
#include <coalpy.tasks/ITaskSystem.h>
#include <coalpy.files/IFileSystem.h>
#include <sstream>
#include <iostream>

namespace coalpy
{

class FileSystemContext : public TestContext
{
public:
    ITaskSystem* ts = nullptr;
    IFileSystem* fs = nullptr;

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

void testCreateDeleteDir(TestContext& ctx)
{
    auto& testContext = (FileSystemContext&)ctx;
    testContext.begin();
    IFileSystem& fs = *testContext.fs;
    const char* dir = "test/some/dir";
    {
        bool result = fs.carveDirectoryPath(dir);
        CPY_ASSERT_MSG(result, "carveDirectoryPath");
    }

    {
        bool result = fs.deleteDirectory(dir);
        CPY_ASSERT_MSG(result, "deleteDirectory");
    }


    testContext.end();
}

void testFileReadWrite(TestContext& ctx)
{
    auto& testContext = (FileSystemContext&)ctx;
    testContext.begin();

    IFileSystem& fs = *testContext.fs;
    
    std::string str = "hello world!";
    bool writeSuccess = false;
    AsyncFileHandle writeHandle = fs.write(FileWriteRequest {
        "test.txt",
        [&writeSuccess](FileWriteResponse& response)
        {
            if (response.status == FileStatus::WriteSuccess)
                writeSuccess = true;
        },
        str.c_str(),
        (int)str.size()
    });

    fs.wait(writeHandle);
    fs.closeHandle(writeHandle);
    CPY_ASSERT(writeSuccess);

    bool deleteSuccess = fs.deleteFile("test.txt");
    CPY_ASSERT(deleteSuccess);
    
    testContext.end();
}

class FileSystemTestSuite : public TestSuite
{
public:
    virtual const char* name() const { return "filesystem"; }
    virtual const TestCase* getCases(int& caseCounts) const
    {
        static TestCase sCases[] = {
            { "createDeleteDir", testCreateDeleteDir },
            { "fileReadWrite", testFileReadWrite }
        };

        caseCounts = (int)(sizeof(sCases) / sizeof(TestCase));
        return sCases;
    }

    virtual TestContext* createContext()
    {
        auto testContext = new FileSystemContext();

        {
            TaskSystemDesc desc;
            desc.threadPoolSize = 8;
            testContext->ts = ITaskSystem::create(desc);
        }

        {
            FileSystemDesc desc { testContext->ts };
            testContext->fs = IFileSystem::create(desc);
        }

        return testContext;
    }

    virtual void destroyContext(TestContext* context)
    {
        auto testContext = static_cast<FileSystemContext*>(context);
        delete testContext;
    }
};

TestSuite* fileSystemSuite()
{
    return new FileSystemTestSuite;
}

}
