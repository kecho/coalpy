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

void deleteAllDir(IFileSystem& fs, const char* dir)
{
    std::vector<std::string> outDirs;
    fs.enumerateFiles(dir, outDirs);
    CPY_ASSERT_MSG(!outDirs.empty(), "Found empty directory.");
    for (auto& f : outDirs)
    {
        FileAttributes attributes = {};
        fs.getFileAttributes(f.c_str(), attributes);
        if (!attributes.isDir)
        {
            bool deleteResult = fs.deleteFile(f.c_str());
            CPY_ASSERT(deleteResult);
        }
        else if (!attributes.isDot)
        {
            bool deleteResult = fs.deleteDirectory(f.c_str());
            CPY_ASSERT(deleteResult);
        }
    }
    
    bool deleteTestDirResult = fs.deleteDirectory(dir);
    CPY_ASSERT(deleteTestDirResult);
}

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
    ITaskSystem& ts = *testContext.ts;
    
    std::string str = "hello world!";
    bool writeSuccess = false;
    AsyncFileHandle writeHandle = fs.write(FileWriteRequest(
        ".test_folder/test.txt",
        [&writeSuccess](FileWriteResponse& response)
        {
            if (response.status == FileStatus::WriteSuccess)
                writeSuccess = true;
        },
        str.c_str(),
        (int)str.size()
    ));


    bool readSuccess = false;
    std::string readResult;
    
    AsyncFileHandle readHandle = fs.read(FileReadRequest(
        ".test_folder/test.txt",
        [&readSuccess, &readResult](FileReadResponse& response)
        {
            if (response.status == FileStatus::Reading)
                readResult.append(response.buffer, response.size);
            else if (response.status == FileStatus::ReadingSuccessEof)
                readSuccess = true;
        }
    ));

    ts.depends(fs.asTask(readHandle), fs.asTask(writeHandle));
    ts.execute(fs.asTask(readHandle));
    fs.wait(readHandle);

    CPY_ASSERT(writeSuccess);
    CPY_ASSERT(readSuccess);
    CPY_ASSERT_FMT(readResult == str, "Mismatch result found %s expected %s", readResult.c_str(), str.c_str());
    fs.closeHandle(writeHandle);
    fs.closeHandle(readHandle);

    {
        deleteAllDir(fs, ".test_folder");
    }
    
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
        delete testContext->fs;
        delete testContext->ts;
        delete testContext;
    }
};

TestSuite* fileSystemSuite()
{
    return new FileSystemTestSuite;
}

}
