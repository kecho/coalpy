#pragma once
#include <coalpy.files/IFileSystem.h>
#include <coalpy.tasks/TaskDefs.h>
#include <coalpy.core/ByteBuffer.h>
#include <coalpy.core/HandleContainer.h>
#include "InternalFileSystem.h"
#include <vector>
#include <queue>
#include <variant>
#include <string>
#include <mutex>
#include <shared_mutex>
#include <atomic>

namespace coalpy
{

class FileSystem : public IFileSystem
{
public:
    FileSystem(const FileSystemDesc& desc);
    virtual ~FileSystem();
    virtual AsyncFileHandle read(const FileReadRequest& request) override;
    virtual AsyncFileHandle write(const FileWriteRequest& request) override;
    virtual void execute(AsyncFileHandle handle) override;
    virtual Task asTask(AsyncFileHandle handle) override;
    virtual void wait(AsyncFileHandle handle) override;
    virtual bool readStatus (AsyncFileHandle handle, FileReadResponse& response) override;
    virtual bool writeStatus(AsyncFileHandle handle, FileWriteResponse& response) override;
    virtual void closeHandle(AsyncFileHandle handle) override;
    virtual bool carveDirectoryPath(const char* directoryName) override;
    virtual void enumerateFiles(const char* directoryName, std::vector<std::string>& dirList) override;
    virtual bool deleteDirectory(const char* directoryName) override;
    virtual bool deleteFile(const char* fileName) override;
    virtual void getFileAttributes(const char* fileName, FileAttributes& attributes) override;

private:

    struct Request
    {
        InternalFileSystem::RequestType type = InternalFileSystem::RequestType::Read;
        std::queue<std::string> filenames;
        FileReadDoneCallback readCallback = nullptr;
        FileWriteDoneCallback writeCallback = nullptr;
        InternalFileSystem::OpaqueFileHandle opaqueHandle = {};

        ByteBuffer writeBuffer;
        int writeSize = 0;

        Task task;
        std::atomic<IoError> error;
        std::atomic<FileStatus> fileStatus;
    };

    ITaskSystem& m_ts;
    FileSystemDesc m_desc;
    mutable std::shared_mutex m_requestsMutex;
    HandleContainer<AsyncFileHandle, Request*> m_requests;
};

}
