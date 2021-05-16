#pragma once
#include <coalpy.files/IFileSystem.h>
#include <coalpy.tasks/TaskDefs.h>
#include <coalpy.core/ByteBuffer.h>
#include <coalpy.core/HandleContainer.h>
#include <vector>
#include <variant>
#include <string>
#include <mutex>
#include <shared_mutex>
#include <atomic>

namespace coalpy
{

class FileSystemInternal;

class FileSystem : public IFileSystem
{
public:
    FileSystem(const FileSystemDesc& desc);
    virtual ~FileSystem();
    virtual AsyncFileHandle read(const FileReadRequest& request) override;
    virtual AsyncFileHandle write(const FileWriteRequest& request) override;
    virtual void wait(AsyncFileHandle handle) override;
    virtual bool readStatus (AsyncFileHandle handle, FileReadResponse& response) override;
    virtual bool writeStatus(AsyncFileHandle handle, FileWriteResponse& response) override;
    virtual void closeHandle(AsyncFileHandle handle) override;

    friend class FileSystemInternal;

private:
    enum RequestType { Read, Write };

    typedef void* OpaqueFileHandle;

    struct Request
    {
        RequestType type = RequestType::Read;
        std::string filename;
        FileReadDoneCallback callback;
        OpaqueFileHandle opaqueHandle;

        Task task;
        ByteBuffer buffer;
        std::atomic<FileStatus> fileStatus;
    };

    ITaskSystem& m_ts;
    FileSystemDesc m_desc;
    mutable std::shared_mutex m_requestsMutex;
    HandleContainer<AsyncFileHandle, Request*> m_requests;
    FileSystemInternal& m_internalFs;
};

}
