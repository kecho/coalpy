#pragma once
#include <coalpy.files/IFileSystem.h>
#include <coalpy.core/ByteBuffer.h>
#include <coalpy.core/HandleContainer.h>
#include <vector>
#include <variant>
#include <string>
#include <atomic>

namespace coalpy
{

class FileSystem : public IFileSystem
{
public:
    FileSystem(const FileSystemDesc& desc);
    virtual AsyncFileHandle open(const FileOpenRequest& request);
    virtual AsyncFileHandle write(const FileWriteRequest& request);
    virtual void wait(AsyncFileHandle handle);
    virtual bool openStatus (AsyncFileHandle handle, FileOpenResponse& response);
    virtual bool writeStatus(AsyncFileHandle handle, FileWriteResponse& response);
    virtual void closeHandle(AsyncFileHandle handle);

private:
    enum RequestType { Open, Write };
    struct Request
    {
        AsyncFileHandle handle;
        RequestType type = RequestType::Open;
        ByteBuffer buffer;
        std::string filename;
        std::atomic<FileStatus> fileStatus;
    };

    ITaskSystem& m_ts;
    FileSystemDesc m_desc;
    HandleContainer<AsyncFileHandle, Request> m_requests;

};

}
