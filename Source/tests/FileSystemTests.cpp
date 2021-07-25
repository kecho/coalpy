#include "testsystem.h"
#include <coalpy.core/Assert.h>
#include <coalpy.tasks/ITaskSystem.h>
#include <coalpy.files/IFileSystem.h>
#include <coalpy.files/IFileWatcher.h>
#include <coalpy.files/Utils.h>
#include <unordered_map>
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
            CPY_ASSERT_FMT(response.status != FileStatus::Fail, "writing fail: %s", IoError2String(response.error));
            if (response.status == FileStatus::Success)
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
            CPY_ASSERT_FMT(response.status != FileStatus::Fail, "reading fail: %s", IoError2String(response.error));
            if (response.status == FileStatus::Reading)
                readResult.append(response.buffer, response.size);
            else if (response.status == FileStatus::Success)
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

void testFileWatcher(TestContext& ctx)
{
    auto& testContext = (FileSystemContext&)ctx;
    testContext.begin();
    IFileSystem& fs = *testContext.fs;
    
    bool carveResult = fs.carveDirectoryPath(".testWatchFile");
    CPY_ASSERT(carveResult);
    auto getFileName = [](int i)
    {
        std::stringstream ss;
        ss << ".testWatchFile/file-" << i << ".txt";
        std::string fileName;
        FileUtils::fixStringPath(ss.str(), fileName); 
        return fileName;
    };

    auto writeFile = [&fs](const std::string& fileName, int* success)
    {
        std::string testString = "hello world";
        *success = 0;
        return fs.write(FileWriteRequest(fileName, [success](FileWriteResponse& response)
        {
            CPY_ASSERT_FMT(response.status != FileStatus::Fail, "writing fail: %s", IoError2String(response.error));
            if (response.status == FileStatus::Success)
                *success = 1;
            else if (response.status == FileStatus::Fail)
                CPY_ASSERT(false);
        }, testString.data(), testString.size()));
    };

    
    int numFiles = 20;
    using FileWritesMap = std::unordered_map<std::string, int>;
    FileWritesMap fileWrites;
    std::vector<int> successes;
    std::vector<AsyncFileHandle> handles;
    handles.resize(numFiles);
    successes.resize(numFiles);
    for (int i = 0; i < numFiles; ++i)
    {
        std::string fileName = getFileName(i);
        std::string resolvedPath;
        FileUtils::getAbsolutePath(fileName, resolvedPath);
        handles[i] = writeFile(fileName, &successes[i]);
        fs.execute(handles[i]);
        fileWrites[resolvedPath] = 0;
    }

    for (int i = 0; i < numFiles; ++i)
    {
        fs.wait(handles[i]);
        fs.closeHandle(handles[i]);
        std::string fileName = getFileName(i);
        CPY_ASSERT_FMT(successes[i], "Failed writting file \"%s\"", fileName.c_str());
    }
    
    FileWatchDesc fwdesc { 100 };
    IFileWatcher& fileWatcher = *IFileWatcher::create(fwdesc);
    fileWatcher.start();

    class WatchObj : public IFileWatchListener
    {
    public:
        WatchObj(FileWritesMap& map) : m_map(map) {}
        virtual ~WatchObj() {}
        virtual void onFilesChanged(const std::set<std::string>& filesChanged)
        {
            for (auto& fn : filesChanged)
            {
                std::string resolvedPath;
                FileUtils::getAbsolutePath(fn, resolvedPath);
                ++m_map[resolvedPath];
            }
        }

    private:
        FileWritesMap& m_map;
    };

    WatchObj watchObj(fileWrites);
    fileWatcher.addListener(&watchObj); 
    fileWatcher.addDirectory(".testWatchFile");

    for (int i = 0; i < numFiles; ++i)
    {
        std::string fileName = getFileName(i);
        handles[i] = writeFile(fileName, &successes[i]);
        fs.execute(handles[i]);
    }

    for (int i = 0; i < numFiles; ++i)
    {
        fs.wait(handles[i]);
        fs.closeHandle(handles[i]);
        std::string fileName = getFileName(i);
        CPY_ASSERT_FMT(successes[i], "Failed writting file \"%s\"", fileName.c_str());
    }

    fileWatcher.stop();
    delete &fileWatcher;

    for (auto& it : fileWrites)
    {
        CPY_ASSERT_FMT(it.second > 0, "Did not detected a write for file \"%s\"", it.first.c_str());
    }
    
    deleteAllDir(fs, ".testWatchFile");
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
            { "fileReadWrite", testFileReadWrite },
            { "fileWatcher", testFileWatcher }
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
